#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/transform_optional_argument_functions.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <string>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class ReplacePresentCalls: public ASR::BaseExprReplacer<ReplacePresentCalls> {

    private:

    Allocator& al;
    ASR::Function_t* f;

    public:

    ReplacePresentCalls(Allocator& al_, ASR::Function_t* f_) : al(al_), f(f_)
    {}

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        ASR::symbol_t* x_sym = x->m_name;
        bool replace_func_call = false;
        if( ASR::is_a<ASR::ExternalSymbol_t>(*x_sym) ) {
            ASR::ExternalSymbol_t* x_ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(x_sym);
            if( std::string(x_ext_sym->m_module_name) == "lfortran_intrinsic_builtin" &&
                std::string(x_ext_sym->m_name) == "present" ) {
                replace_func_call = true;
            }
        }

        if( !replace_func_call ) {
            return ;
        }

        ASR::symbol_t* present_arg = ASR::down_cast<ASR::Var_t>(x->m_args[0].m_value)->m_v;
        size_t i;
        for( i = 0; i < f->n_args; i++ ) {
            if( ASR::down_cast<ASR::Var_t>(f->m_args[i])->m_v == present_arg ) {
                i++;
                break;
            }
        }

        *current_expr = ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc,
                            ASR::down_cast<ASR::Var_t>(f->m_args[i])->m_v));
    }

};


class ReplacePresentCallsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplacePresentCallsVisitor>
{
    private:

        ReplacePresentCalls replacer;

    public:

        ReplacePresentCallsVisitor(Allocator& al_,
            ASR::Function_t* f_) : replacer(al_, f_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

class TransformFunctionsWithOptionalArguments: public PassUtils::PassVisitor<TransformFunctionsWithOptionalArguments> {

    public:

        std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx;

        TransformFunctionsWithOptionalArguments(Allocator &al_,
            std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx_) :
        PassVisitor(al_, nullptr), sym2optionalargidx(sym2optionalargidx_)
        {
            pass_result.reserve(al, 1);
        }

        void transform_functions_with_optional_arguments(ASR::Function_t* s) {
            Vec<ASR::expr_t*> new_args;
            Vec<ASR::ttype_t*> new_arg_types;
            new_arg_types.reserve(al, s->n_args);
            new_args.reserve(al, s->n_args);
            ASR::ttype_t* logical_type = ASRUtils::TYPE(ASR::make_Logical_t(al, s->base.base.loc, 4));
            for( size_t i = 0; i < s->n_args; i++ ) {
                ASR::symbol_t* arg_sym = ASR::down_cast<ASR::Var_t>(s->m_args[i])->m_v;
                new_args.push_back(al, s->m_args[i]);
                new_arg_types.push_back(al, ASRUtils::get_FunctionType(*s)->m_arg_types[i]);
                if( is_presence_optional(arg_sym, true) ) {
                    sym2optionalargidx[&(s->base)].push_back(new_args.size() - 1);
                    std::string presence_bit_arg_name = "is_" + std::string(ASRUtils::symbol_name(arg_sym)) + "_present_";
                    presence_bit_arg_name = s->m_symtab->get_unique_name(presence_bit_arg_name, false);
                    ASR::expr_t* presence_bit_arg = PassUtils::create_auxiliary_variable(
                                                        arg_sym->base.loc, presence_bit_arg_name,
                                                        al, s->m_symtab, logical_type, ASR::intentType::In);
                    new_args.push_back(al, presence_bit_arg);
                    new_arg_types.push_back(al, logical_type);
                }
            }

            ASR::FunctionType_t* function_type = ASRUtils::get_FunctionType(*s);
            function_type->m_arg_types = new_arg_types.p;
            function_type->n_arg_types = new_arg_types.size();
            s->m_args = new_args.p;
            s->n_args = new_args.size();
            ReplacePresentCallsVisitor present_replacer(al, s);
            present_replacer.visit_Function(*s);
        }

        bool is_presence_optional(ASR::symbol_t* sym, bool set_presence_to_required=false) {
            if( ASR::is_a<ASR::Variable_t>(*sym) ) {
                ASR::Variable_t* sym_ = ASR::down_cast<ASR::Variable_t>(sym);
                if (sym_->m_presence == ASR::presenceType::Optional) {
                    if( set_presence_to_required ) {
                        sym_->m_presence = ASR::presenceType::Required;
                    }
                    return true;
                }
            }
            return false;
        }

        bool is_optional_argument_present(ASR::Function_t* s) {
            for( size_t i = 0; i < s->n_args; i++ ) {
                ASR::symbol_t* arg_sym = ASR::down_cast<ASR::Var_t>(s->m_args[i])->m_v;
                if( is_presence_optional(arg_sym) ) {
                    return true;
                }
            }
            return false;
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (is_optional_argument_present(s)) {
                        transform_functions_with_optional_arguments(s);
                    }
                }
            }

            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
                this->visit_symbol(*item.second);
            }
        }

        void visit_Program(const ASR::Program_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (is_optional_argument_present(s)) {
                        transform_functions_with_optional_arguments(s);
                    }
                }
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    visit_Function(*s);
                }
            }

            current_scope = xx.m_symtab;
            transform_stmts(xx.m_body, xx.n_body);

        }

        void visit_Module(const ASR::Module_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Module_t &xx = const_cast<ASR::Module_t&>(x);
            current_scope = xx.m_symtab;

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (is_optional_argument_present(s)) {
                        transform_functions_with_optional_arguments(s);
                    }
                }
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    visit_Function(*s);
                }
            }

        }

        void visit_Function(const ASR::Function_t &x) {
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
            current_scope = xx.m_symtab;

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (is_optional_argument_present(s)) {
                        transform_functions_with_optional_arguments(s);
                    }
                }
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    visit_Function(*s);
                }
            }

        }

};

template <typename T>
bool fill_new_args(Vec<ASR::call_arg_t>& new_args, Allocator& al,
    const T& x, SymbolTable* scope, std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx) {
    ASR::Function_t* owning_function = nullptr;
    if( scope->asr_owner && ASR::is_a<ASR::symbol_t>(*scope->asr_owner) &&
        ASR::is_a<ASR::Function_t>(*ASR::down_cast<ASR::symbol_t>(scope->asr_owner)) ) {
        owning_function = ASR::down_cast<ASR::Function_t>(
            ASR::down_cast<ASR::symbol_t>(scope->asr_owner));
    }
    ASR::symbol_t* func_sym = ASRUtils::symbol_get_past_external(x.m_name);
    if( !ASR::is_a<ASR::Function_t>(*func_sym) ) {
        return false;
    }

    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(func_sym);
    bool replace_func_call = false;
    for( size_t i = 0; i < func->n_args; i++ ) {
        if (std::find(sym2optionalargidx[func_sym].begin(),
                      sym2optionalargidx[func_sym].end(), i)
            != sym2optionalargidx[func_sym].end()) {
            replace_func_call = true;
            break ;
        }
    }

    if( !replace_func_call ) {
        return false;
    }

    new_args.reserve(al, func->n_args);
    for( size_t i = 0, j = 0; j < func->n_args; j++, i++ ) {
        LCOMPILERS_ASSERT(i < x.n_args);
        if( std::find(sym2optionalargidx[func_sym].begin(),
                      sym2optionalargidx[func_sym].end(), j)
            != sym2optionalargidx[func_sym].end() ) {
            ASR::Variable_t* func_arg_j = ASRUtils::EXPR2VAR(func->m_args[j]);
            if( x.m_args[i].m_value == nullptr ) {
                std::string m_arg_i_name = scope->get_unique_name("__libasr_created_variable_");
                ASR::ttype_t* arg_type = func_arg_j->m_type;
                if( ASR::is_a<ASR::Array_t>(*arg_type) ) {
                    ASR::Array_t* array_t = ASR::down_cast<ASR::Array_t>(arg_type);
                    Vec<ASR::dimension_t> dims;
                    dims.reserve(al, array_t->n_dims);
                    for( size_t i = 0; i < array_t->n_dims; i++ ) {
                        ASR::dimension_t dim;
                        dim.m_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arg_type->base.loc, 1,
                                            ASRUtils::TYPE(ASR::make_Integer_t(al, arg_type->base.loc, 4))));
                        dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, arg_type->base.loc, 1,
                                            ASRUtils::TYPE(ASR::make_Integer_t(al, arg_type->base.loc, 4))));
                        dim.loc = arg_type->base.loc;
                        dims.push_back(al, dim);
                    }
                    arg_type = ASRUtils::TYPE(ASR::make_Array_t(al, arg_type->base.loc,
                                array_t->m_type, dims.p, dims.size(), ASR::array_physical_typeType::FixedSizeArray));
                }
                ASR::expr_t* m_arg_i = PassUtils::create_auxiliary_variable(
                    x.m_args[i].loc, m_arg_i_name, al, scope, arg_type);
                arg_type = ASRUtils::expr_type(m_arg_i);
                if( ASRUtils::is_array(arg_type) &&
                    ASRUtils::extract_physical_type(arg_type) !=
                    ASRUtils::extract_physical_type(func_arg_j->m_type)) {
                    ASR::ttype_t* m_type = ASRUtils::duplicate_type(al, arg_type, nullptr,
                        ASRUtils::extract_physical_type(func_arg_j->m_type), true);
                    m_arg_i = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(
                        al, m_arg_i->base.loc, m_arg_i, ASRUtils::extract_physical_type(arg_type),
                        ASRUtils::extract_physical_type(func_arg_j->m_type), m_type, nullptr));
                }
                ASR::call_arg_t m_call_arg_i;
                m_call_arg_i.loc = x.m_args[i].loc;
                m_call_arg_i.m_value = m_arg_i;
                new_args.push_back(al, m_call_arg_i);
            } else {
                new_args.push_back(al, x.m_args[i]);
            }
            ASR::ttype_t* logical_t = ASRUtils::TYPE(ASR::make_Logical_t(al,
                                        x.m_args[i].loc, 4));
            ASR::expr_t* is_present = nullptr;
            if( x.m_args[i].m_value == nullptr ) {
                is_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(
                    al, x.m_args[i].loc, false, logical_t));
            } else {
                if( owning_function != nullptr ) {
                    size_t k;
                    bool k_found = false;
                    for( k = 0; k < owning_function->n_args; k++ ) {
                        if( ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value) && ASR::down_cast<ASR::Var_t>(owning_function->m_args[k])->m_v ==
                            ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v ) {
                            k_found = true;
                            break ;
                        }
                    }

                    if( k_found && std::find(sym2optionalargidx[&(owning_function->base)].begin(),
                            sym2optionalargidx[&(owning_function->base)].end(), k)
                                != sym2optionalargidx[&(owning_function->base)].end() ) {
                        is_present = owning_function->m_args[k + 1];
                    }
                }

                if( is_present == nullptr ) {
                    is_present = ASRUtils::EXPR(ASR::make_LogicalConstant_t(
                        al, x.m_args[i].loc, true, logical_t));
                }
            }
            ASR::call_arg_t present_arg;
            present_arg.loc = x.m_args[i].loc;
            present_arg.m_value = is_present;
            new_args.push_back(al, present_arg);
            j++;
        } else {
            new_args.push_back(al, x.m_args[i]);
        }
    }
    LCOMPILERS_ASSERT(func->n_args == new_args.size());
    return true;
}

class ReplaceFunctionCallsWithOptionalArguments: public ASR::BaseExprReplacer<ReplaceFunctionCallsWithOptionalArguments> {

    private:

    Allocator& al;
    std::set<ASR::expr_t*> new_func_calls;

    public:

    std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx;
    SymbolTable* current_scope;

    ReplaceFunctionCallsWithOptionalArguments(Allocator& al_,
        std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx_) :
        al(al_), sym2optionalargidx(sym2optionalargidx_), current_scope(nullptr)
    {}

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        Vec<ASR::call_arg_t> new_args;
        if( !fill_new_args(new_args, al, *x, current_scope, sym2optionalargidx) ||
            new_func_calls.find(*current_expr) != new_func_calls.end() ) {
            return ;
        }
        *current_expr = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(al,
                            x->base.base.loc, x->m_name, x->m_original_name,
                            new_args.p, new_args.size(), x->m_type, x->m_value,
                            x->m_dt));
        new_func_calls.insert(*current_expr);
    }

};


class ReplaceFunctionCallsWithOptionalArgumentsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallsWithOptionalArgumentsVisitor>
{
    private:

        ReplaceFunctionCallsWithOptionalArguments replacer;

    public:

        ReplaceFunctionCallsWithOptionalArgumentsVisitor(Allocator& al_,
            std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx_) :
        replacer(al_, sym2optionalargidx_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

};

class ReplaceSubroutineCallsWithOptionalArgumentsVisitor : public PassUtils::PassVisitor<ReplaceSubroutineCallsWithOptionalArgumentsVisitor>
{

    public:

        std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx;

        ReplaceSubroutineCallsWithOptionalArgumentsVisitor(Allocator& al_,
            std::map<ASR::symbol_t*, std::vector<int32_t>>& sym2optionalargidx_):
        PassVisitor(al_, nullptr), sym2optionalargidx(sym2optionalargidx_)
        {
            pass_result.reserve(al, 1);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            Vec<ASR::call_arg_t> new_args;
            if( !fill_new_args(new_args, al, x, current_scope, sym2optionalargidx) ) {
                return ;
            }
            pass_result.push_back(al, ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al,
                                    x.base.base.loc, x.m_name, x.m_original_name,
                                    new_args.p, new_args.size(), x.m_dt,
                                    nullptr, false)));
        }
};

void pass_transform_optional_argument_functions(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    std::map<ASR::symbol_t*, std::vector<int32_t>> sym2optionalargidx;
    TransformFunctionsWithOptionalArguments v(al, sym2optionalargidx);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallsWithOptionalArgumentsVisitor w(al, sym2optionalargidx);
    w.visit_TranslationUnit(unit);
    ReplaceSubroutineCallsWithOptionalArgumentsVisitor y(al, sym2optionalargidx);
    y.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LFortran
