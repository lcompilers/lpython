#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/create_subroutine_from_function.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <string>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class CreateFunctionFromSubroutine: public PassUtils::PassVisitor<CreateFunctionFromSubroutine> {

    public:

        CreateFunctionFromSubroutine(Allocator &al_) :
        PassVisitor(al_, nullptr)
        {
            pass_result.reserve(al, 1);
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            // Transform functions returning arrays to subroutines
            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_aggregate_or_array_type);
                }
            }

            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_symtab->get_symbol(item));
                ASR::symbol_t *mod = x.m_symtab->get_symbol(item);
                visit_symbol(*mod);
            }
            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
                if (!ASR::is_a<ASR::Module_t>(*item.second)) {
                    this->visit_symbol(*item.second);
                }
            }
        }

        void visit_Module(const ASR::Module_t &x) {
            current_scope = x.m_symtab;
            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_aggregate_or_array_type);
                }
            }

            // Now visit everything else
            for (auto &item : x.m_symtab->get_scope()) {
                this->visit_symbol(*item.second);
            }
        }

        void visit_Program(const ASR::Program_t &x) {
            std::vector<std::pair<std::string, ASR::symbol_t*> > replace_vec;
            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
            current_scope = xx.m_symtab;

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_aggregate_or_array_type);
                }
            }

            for (auto &item : x.m_symtab->get_scope()) {
                if (is_a<ASR::AssociateBlock_t>(*item.second)) {
                    ASR::AssociateBlock_t *s = ASR::down_cast<ASR::AssociateBlock_t>(item.second);
                    visit_AssociateBlock(*s);
                }
                if (is_a<ASR::Function_t>(*item.second)) {
                    visit_Function(*ASR::down_cast<ASR::Function_t>(item.second));
                }
            }

            current_scope = xx.m_symtab;
            transform_stmts(xx.m_body, xx.n_body);

        }

};

class ReplaceFunctionCallWithSubroutineCall:
    public ASR::BaseExprReplacer<ReplaceFunctionCallWithSubroutineCall> {

    private:

        Allocator& al;
        int result_counter;
        Vec<ASR::stmt_t*>& pass_result;
        std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value;

    public:

        SymbolTable* current_scope;
        ASR::expr_t* result_var;
        bool& apply_again;

        ReplaceFunctionCallWithSubroutineCall(Allocator& al_,
            Vec<ASR::stmt_t*>& pass_result_,
            std::map<ASR::expr_t*, ASR::expr_t*>& resultvar2value_,
            bool& apply_again_):
        al(al_), result_counter(0), pass_result(pass_result_),
        resultvar2value(resultvar2value_), result_var(nullptr),
        apply_again(apply_again_) {}

        void replace_FunctionCall(ASR::FunctionCall_t* x) {
            // The following checks if the name of a function actually
            // points to a subroutine. If true this would mean that the
            // original function returned an array and is now a subroutine.
            // So the current function call will be converted to a subroutine
            // call. In short, this check acts as a signal whether to convert
            // a function call to a subroutine call.
            if (current_scope == nullptr) {
                return ;
            }

            const Location& loc = x->base.base.loc;
            if( ASR::is_a<ASR::Function_t>(*ASRUtils::symbol_get_past_external(x->m_name)) &&
                ASRUtils::symbol_abi(x->m_name) == ASR::abiType::Source ) {
                for( size_t i = 0; i < x->n_args; i++ ) {
                    if( x->m_args[i].m_value && ASR::is_a<ASR::ArrayPhysicalCast_t>(*x->m_args[i].m_value) &&
                        ASR::is_a<ASR::FunctionCall_t>(*
                            ASR::down_cast<ASR::ArrayPhysicalCast_t>(x->m_args[i].m_value)->m_arg) ) {
                        x->m_args[i].m_value = ASR::down_cast<ASR::ArrayPhysicalCast_t>(x->m_args[i].m_value)->m_arg;
                    }
                    if( x->m_args[i].m_value && ASR::is_a<ASR::FunctionCall_t>(*x->m_args[i].m_value) &&
                        ASRUtils::is_array(ASRUtils::expr_type(x->m_args[i].m_value)) ) {
                        ASR::expr_t* arg_var = PassUtils::create_var(result_counter,
                            "_func_call_arg_tmp_", loc, x->m_args[i].m_value, al, current_scope);
                        result_counter += 1;
                        apply_again = true;
                        pass_result.push_back(al, ASRUtils::STMT(ASR::make_Assignment_t(al, loc,
                            arg_var, x->m_args[i].m_value, nullptr)));
                        x->m_args[i].m_value = arg_var;
                    }
                }
            }

            if (x->m_value) {
                *current_expr = x->m_value;
                return;
            }

            ASR::expr_t* result_var_ = nullptr;
            if( resultvar2value.find(result_var) != resultvar2value.end() &&
                resultvar2value[result_var] == *current_expr ) {
                result_var_ = result_var;
            }

            bool is_return_var_handled = false;
            ASR::symbol_t *fn_name = ASRUtils::symbol_get_past_external(x->m_name);
            if (ASR::is_a<ASR::Function_t>(*fn_name)) {
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
                is_return_var_handled = fn->m_return_var == nullptr;
            }
            if (is_return_var_handled) {
                ASR::ttype_t* result_var_type = ASRUtils::duplicate_type(al, x->m_type);
                bool is_allocatable = false;
                bool is_func_call_allocatable = false;
                bool is_result_var_allocatable = false;
                bool is_created_result_var_type_dependent_on_local_vars = false;
                ASR::dimension_t* m_dims_ = nullptr;
                size_t n_dims_ = 0;
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
                {
                    // Assuming the `m_return_var` is appended to the `args`.
                    ASR::symbol_t *v_sym = ASR::down_cast<ASR::Var_t>(
                        fn->m_args[fn->n_args-1])->m_v;
                    if (ASR::is_a<ASR::Variable_t>(*v_sym)) {
                        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(v_sym);
                        is_func_call_allocatable = ASR::is_a<ASR::Allocatable_t>(*v->m_type);
                        if( result_var_ != nullptr ) {
                            is_result_var_allocatable = ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(result_var_));
                            is_allocatable = is_func_call_allocatable || is_result_var_allocatable;
                        }
                        n_dims_ = ASRUtils::extract_dimensions_from_ttype(result_var_type, m_dims_);
                        is_created_result_var_type_dependent_on_local_vars = !ASRUtils::is_dimension_dependent_only_on_arguments(m_dims_, n_dims_);
                        if( is_allocatable || is_created_result_var_type_dependent_on_local_vars ) {
                            result_var_type = ASRUtils::duplicate_type_with_empty_dims(al, result_var_type);
                            result_var_type = ASRUtils::TYPE(ASR::make_Allocatable_t(
                                al, loc, ASRUtils::type_get_past_allocatable(
                                ASRUtils::type_get_past_pointer(result_var_type))));
                        }
                    }

                    // Don't always create this temporary variable
                    ASR::expr_t* result_var__ = PassUtils::create_var(result_counter,
                        "_func_call_res", loc, result_var_type, al, current_scope);
                    result_counter += 1;
                    *current_expr = result_var__;
                }

                if( !is_func_call_allocatable && is_result_var_allocatable ) {
                    Vec<ASR::alloc_arg_t> vec_alloc;
                    vec_alloc.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg;
                    alloc_arg.m_len_expr = nullptr;
                    alloc_arg.m_type = nullptr;
                    alloc_arg.loc = loc;
                    alloc_arg.m_a = *current_expr;

                    ASR::FunctionType_t* fn_type = ASRUtils::get_FunctionType(fn);
                    ASR::ttype_t* output_type = fn_type->m_arg_types[fn_type->n_arg_types - 1];
                    ASR::dimension_t* m_dims = nullptr;
                    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(output_type, m_dims);
                    Vec<ASR::dimension_t> vec_dims;
                    vec_dims.reserve(al, n_dims);
                    ASRUtils::ReplaceFunctionParamVisitor replace_function_param_visitor(x->m_args);
                    ASRUtils::ExprStmtDuplicator expr_duplicator(al);
                    for( size_t i = 0; i < n_dims; i++ ) {
                        ASR::dimension_t dim;
                        dim.loc = loc;
                        dim.m_start = expr_duplicator.duplicate_expr(m_dims[i].m_start);
                        dim.m_length = expr_duplicator.duplicate_expr(m_dims[i].m_length);
                        replace_function_param_visitor.current_expr = &dim.m_start;
                        replace_function_param_visitor.replace_expr(dim.m_start);
                        replace_function_param_visitor.current_expr = &dim.m_length;
                        replace_function_param_visitor.replace_expr(dim.m_length);
                        vec_dims.push_back(al, dim);
                    }

                    alloc_arg.m_dims = vec_dims.p;
                    alloc_arg.n_dims = vec_dims.n;
                    vec_alloc.push_back(al, alloc_arg);
                    Vec<ASR::expr_t*> to_be_deallocated;
                    to_be_deallocated.reserve(al, vec_alloc.size());
                    for( size_t i = 0; i < vec_alloc.size(); i++ ) {
                        to_be_deallocated.push_back(al, vec_alloc.p[i].m_a);
                    }
                    pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                        al, loc, to_be_deallocated.p, to_be_deallocated.size())));
                    pass_result.push_back(al, ASRUtils::STMT(ASR::make_Allocate_t(
                        al, loc, vec_alloc.p, 1, nullptr, nullptr, nullptr)));
                } else if( !is_func_call_allocatable && is_created_result_var_type_dependent_on_local_vars ) {
                    Vec<ASR::dimension_t> alloc_dims;
                    alloc_dims.reserve(al, n_dims_);
                    for( size_t i = 0; i < n_dims_; i++ ) {
                        ASR::dimension_t alloc_dim;
                        alloc_dim.loc = loc;
                        alloc_dim.m_start = make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc);
                        if( m_dims_[i].m_length ) {
                            alloc_dim.m_length = m_dims_[i].m_length;
                        } else {
                            alloc_dim.m_length = ASRUtils::get_size(result_var, i + 1, al);
                        }
                        alloc_dims.push_back(al, alloc_dim);
                    }
                    Vec<ASR::alloc_arg_t> alloc_args;
                    alloc_args.reserve(al, 1);
                    ASR::alloc_arg_t alloc_arg;
                    alloc_arg.loc = loc;
                    alloc_arg.m_len_expr = nullptr;
                    alloc_arg.m_type = nullptr;
                    alloc_arg.m_a = *current_expr;
                    alloc_arg.m_dims = alloc_dims.p;
                    alloc_arg.n_dims = alloc_dims.size();
                    alloc_args.push_back(al, alloc_arg);
                    Vec<ASR::expr_t*> to_be_deallocated;
                    to_be_deallocated.reserve(al, alloc_args.size());
                    for( size_t i = 0; i < alloc_args.size(); i++ ) {
                        to_be_deallocated.push_back(al, alloc_args.p[i].m_a);
                    }
                    pass_result.push_back(al, ASRUtils::STMT(ASR::make_ExplicitDeallocate_t(
                        al, loc, to_be_deallocated.p, to_be_deallocated.size())));
                    pass_result.push_back(al, ASRUtils::STMT(ASR::make_Allocate_t(al,
                        loc, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr)));
                }

                Vec<ASR::call_arg_t> s_args;
                s_args.reserve(al, x->n_args + 1);
                for( size_t i = 0; i < x->n_args; i++ ) {
                    ASR::expr_t** current_expr_copy_9 = current_expr;
                    current_expr = &(x->m_args[i].m_value);
                    self().replace_expr(x->m_args[i].m_value);
                    current_expr = current_expr_copy_9;
                    s_args.push_back(al, x->m_args[i]);
                }
                ASR::call_arg_t result_arg;
                result_arg.loc = loc;
                result_arg.m_value = *current_expr;
                s_args.push_back(al, result_arg);
                ASR::stmt_t* subrout_call = ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, loc,
                                                x->m_name, nullptr, s_args.p, s_args.size(), nullptr,
                                                nullptr, false, false));
                pass_result.push_back(al, subrout_call);
            }
        }

        void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
            ASR::BaseExprReplacer<ReplaceFunctionCallWithSubroutineCall>::replace_ArrayPhysicalCast(x);
            if( (x->m_old == x->m_new &&
                x->m_old != ASR::array_physical_typeType::DescriptorArray) ||
                (x->m_old == x->m_new && x->m_old == ASR::array_physical_typeType::DescriptorArray &&
                (ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(x->m_arg)) ||
                ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x->m_arg)))) ||
                x->m_old != ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg)) ) {
                *current_expr = x->m_arg;
            } else {
                x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
            }
        }

};

class ReplaceFunctionCallWithSubroutineCallVisitor:
    public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor> {

    private:

        Allocator& al;
        Vec<ASR::stmt_t*> pass_result;
        ReplaceFunctionCallWithSubroutineCall replacer;
        Vec<ASR::stmt_t*>* parent_body;
        std::map<ASR::expr_t*, ASR::expr_t*> resultvar2value;

    public:

        bool apply_again;

        ReplaceFunctionCallWithSubroutineCallVisitor(Allocator& al_):
         al(al_), replacer(al, pass_result, resultvar2value, apply_again),
         parent_body(nullptr), apply_again(false)
        {
            pass_result.n = 0;
        }

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            if( parent_body ) {
                for (size_t j=0; j < pass_result.size(); j++) {
                    parent_body->push_back(al, pass_result[j]);
                }
            }
            for (size_t i=0; i<n_body; i++) {
                pass_result.n = 0;
                pass_result.reserve(al, 1);
                Vec<ASR::stmt_t*>* parent_body_copy = parent_body;
                parent_body = &body;
                visit_stmt(*m_body[i]);
                parent_body = parent_body_copy;
                for (size_t j=0; j < pass_result.size(); j++) {
                    body.push_back(al, pass_result[j]);
                }
                body.push_back(al, m_body[i]);
            }
            m_body = body.p;
            n_body = body.size();
            pass_result.n = 0;
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            if( (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_target)) &&
                ASR::is_a<ASR::GetPointer_t>(*x.m_value)) ||
                (ASR::is_a<ASR::ArrayConstant_t>(*x.m_value) ||
                ASR::is_a<ASR::ArrayConstructor_t>(*x.m_value)) ) {
                return ;
            }

            if( ASR::is_a<ASR::ArrayReshape_t>(*x.m_value) ) {
                ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor>::visit_Assignment(x);
                return ;
            }

            if( PassUtils::is_array(x.m_target)
                    || ASR::is_a<ASR::ArraySection_t>(*x.m_target)) {
                replacer.result_var = x.m_target;
                ASR::expr_t* original_value = x.m_value;
                resultvar2value[replacer.result_var] = original_value;
            }
            ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor>::visit_Assignment(x);
        }
};

void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                          const LCompilers::PassOptions& /*pass_options*/) {
    CreateFunctionFromSubroutine v(al);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallWithSubroutineCallVisitor u(al);
    u.apply_again = true;
    while( u.apply_again ) {
        u.apply_again = false;
        u.visit_TranslationUnit(unit);
    }
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
