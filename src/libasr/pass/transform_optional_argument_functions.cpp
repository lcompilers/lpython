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

        TransformFunctionsWithOptionalArguments(Allocator &al_) :
        PassVisitor(al_, nullptr)
        {
            pass_result.reserve(al, 1);
        }

        void transform_functions_with_optional_arguments(ASR::Function_t* s) {
            Vec<ASR::expr_t*> new_args;
            new_args.reserve(al, s->n_args);
            ASR::ttype_t* logical_type = ASRUtils::TYPE(ASR::make_Logical_t(al, s->base.base.loc, 4, nullptr, 0));
            for( size_t i = 0; i < s->n_args; i++ ) {
                ASR::symbol_t* arg_sym = ASR::down_cast<ASR::Var_t>(s->m_args[i])->m_v;
                new_args.push_back(al, s->m_args[i]);
                if( is_presence_optional(arg_sym) ) {
                    std::string presence_bit_arg_name = "is_" + std::string(ASRUtils::symbol_name(arg_sym)) + "_present_";
                    presence_bit_arg_name = s->m_symtab->get_unique_name(presence_bit_arg_name);
                    ASR::expr_t* presence_bit_arg = PassUtils::create_auxiliary_variable(
                                                        arg_sym->base.loc, presence_bit_arg_name,
                                                        al, s->m_symtab, logical_type, ASR::intentType::In);
                    new_args.push_back(al, presence_bit_arg);
                }
            }
            s->m_args = new_args.p;
            s->n_args = new_args.size();
            ReplacePresentCallsVisitor present_replacer(al, s);
            present_replacer.visit_Function(*s);
        }

        bool is_presence_optional(ASR::symbol_t* sym) {
            if( ASR::is_a<ASR::Variable_t>(*sym) ) {
                if (ASR::down_cast<ASR::Variable_t>(sym)->m_presence
                        == ASR::presenceType::Optional) {
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
            for (auto &item : x.m_global_scope->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (is_optional_argument_present(s)) {
                        transform_functions_with_optional_arguments(s);
                    }
                }
            }

            // Now visit everything else
            for (auto &item : x.m_global_scope->get_scope()) {
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

};

template <typename T>
bool fill_new_args(Vec<ASR::call_arg_t>& new_args, Allocator& al, const T& x) {
    ASR::symbol_t* func_sym = ASRUtils::symbol_get_past_external(x.m_name);
    if( !ASR::is_a<ASR::Function_t>(*func_sym) ) {
        return false;
    }

    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(func_sym);
    bool replace_func_call = false;
    for( size_t i = 0; i < func->n_args; i++ ) {
        if (ASR::is_a<ASR::Variable_t>(
                *ASR::down_cast<ASR::Var_t>(func->m_args[i])->m_v) &&
            ASRUtils::EXPR2VAR(func->m_args[i])->m_presence
            == ASR::presenceType::Optional) {
            replace_func_call = true;
            break ;
        }
    }

    if( !replace_func_call ) {
        return false;
    }

    new_args.reserve(al, func->n_args);
    for( size_t i = 0, j = 0; j < func->n_args; i++, j++ ) {
        if( i < x.n_args ) {
            new_args.push_back(al, x.m_args[i]);
        } else {
            ASR::call_arg_t empty_arg;
            Location loc;
            loc.first = 1, loc.last = 1;
            empty_arg.loc = loc;
            empty_arg.m_value = nullptr;
            new_args.push_back(al, empty_arg);
        }
        if( ASR::is_a<ASR::Variable_t>(
                *ASR::down_cast<ASR::Var_t>(func->m_args[j])->m_v) &&
            ASRUtils::EXPR2VAR(func->m_args[j])->m_presence ==
            ASR::presenceType::Optional ) {
            ASR::ttype_t* logical_t = ASRUtils::TYPE(ASR::make_Logical_t(al,
                                        x.m_args[i].loc, 4, nullptr, 0));
            ASR::expr_t* is_present = ASRUtils::EXPR(
                ASR::make_LogicalConstant_t(al, x.m_args[i].loc,
                    x.m_args[i].m_value != nullptr, logical_t));
            ASR::call_arg_t present_arg;
            present_arg.loc = x.m_args[i].loc;
            present_arg.m_value = is_present;
            new_args.push_back(al, present_arg);
            j++;
        }
    }
    LCOMPILERS_ASSERT(func->n_args == new_args.size());
    return true;
}

class ReplaceFunctionCallsWithOptionalArguments: public ASR::BaseExprReplacer<ReplaceFunctionCallsWithOptionalArguments> {

    private:

    Allocator& al;

    public:

    ReplaceFunctionCallsWithOptionalArguments(Allocator& al_) : al(al_)
    {}

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        Vec<ASR::call_arg_t> new_args;
        if( !fill_new_args(new_args, al, *x) ) {
            return ;
        }
        *current_expr = ASRUtils::EXPR(ASR::make_FunctionCall_t(al,
                            x->base.base.loc, x->m_name, x->m_original_name,
                            new_args.p, new_args.size(), x->m_type, x->m_value,
                            x->m_dt));
    }

};


class ReplaceFunctionCallsWithOptionalArgumentsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallsWithOptionalArgumentsVisitor>
{
    private:

        ReplaceFunctionCallsWithOptionalArguments replacer;

    public:

        ReplaceFunctionCallsWithOptionalArgumentsVisitor(Allocator& al_) : replacer(al_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

class ReplaceSubroutineCallsWithOptionalArgumentsVisitor : public PassUtils::PassVisitor<ReplaceSubroutineCallsWithOptionalArgumentsVisitor>
{

    public:

        ReplaceSubroutineCallsWithOptionalArgumentsVisitor(Allocator& al_): PassVisitor(al_, nullptr)
        {
            pass_result.reserve(al, 1);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            Vec<ASR::call_arg_t> new_args;
            if( !fill_new_args(new_args, al, x) ) {
                return ;
            }
            pass_result.push_back(al, ASRUtils::STMT(ASR::make_SubroutineCall_t(al,
                                    x.base.base.loc, x.m_name, x.m_original_name,
                                    new_args.p, new_args.size(), x.m_dt)));
        }
};

void pass_transform_optional_argument_functions(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& /*pass_options*/) {
    TransformFunctionsWithOptionalArguments v(al);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallsWithOptionalArgumentsVisitor w(al);
    w.visit_TranslationUnit(unit);
    ReplaceSubroutineCallsWithOptionalArgumentsVisitor y(al);
    y.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor x(al);
    x.visit_TranslationUnit(unit);
}


} // namespace LFortran
