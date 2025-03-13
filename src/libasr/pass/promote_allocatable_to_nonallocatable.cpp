#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/utils.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/containers.h>
#include <map>

#include <libasr/pass/intrinsic_function_registry.h>

namespace LCompilers {

class IsAllocatedCalled: public ASR::CallReplacerOnExpressionsVisitor<IsAllocatedCalled> {
    public:

        std::map<SymbolTable*, std::vector<ASR::symbol_t*>>& scope2var;

        IsAllocatedCalled(std::map<SymbolTable*, std::vector<ASR::symbol_t*>>& scope2var_):
            scope2var(scope2var_) {}

        void visit_IntrinsicImpureFunction(const ASR::IntrinsicImpureFunction_t& x) {
            if( x.m_impure_intrinsic_id == static_cast<int64_t>(
                ASRUtils::IntrinsicImpureFunctions::Allocated) ) {
                LCOMPILERS_ASSERT(x.n_args == 1);
                if( ASR::is_a<ASR::Var_t>(*x.m_args[0]) ) {
                    scope2var[current_scope].push_back(
                        ASR::down_cast<ASR::Var_t>(x.m_args[0])->m_v);
                }
            }
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            ASR::FunctionType_t* func_type = ASRUtils::get_FunctionType(x.m_name);
            for( size_t i = 0; i < x.n_args; i++ ) {
                if( ASR::is_a<ASR::Allocatable_t>(*func_type->m_arg_types[i]) ||
                    ASR::is_a<ASR::Pointer_t>(*func_type->m_arg_types[i]) ) {
                    if( ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value) ) {
                        scope2var[current_scope].push_back(
                            ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v);
                    }
                }
            }
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::FunctionType_t* func_type = ASRUtils::get_FunctionType(x.m_name);
            for( size_t i = 0; i < x.n_args; i++ ) {
                if( ASR::is_a<ASR::Allocatable_t>(*func_type->m_arg_types[i]) ||
                    ASR::is_a<ASR::Pointer_t>(*func_type->m_arg_types[i]) ) {
                    if( ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value) ) {
                        scope2var[current_scope].push_back(
                            ASR::down_cast<ASR::Var_t>(x.m_args[i].m_value)->m_v);
                    }
                }
            }
        }

        void visit_ReAlloc(const ASR::ReAlloc_t& x) {
            for( size_t i = 0; i < x.n_args; i++ ) {
                if( ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(x.m_args[i].m_a)) ||
                    ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x.m_args[i].m_a)) ) {
                    if( ASR::is_a<ASR::Var_t>(*x.m_args[i].m_a) ) {
                        scope2var[current_scope].push_back(
                            ASR::down_cast<ASR::Var_t>(x.m_args[i].m_a)->m_v);
                    }
                }
            }
        }

        bool is_array_size_called_on_pointer(ASR::dimension_t* m_dims, size_t n_dims) {
            for( size_t i = 0; i < n_dims; i++ ) {
                #define check_pointer_in_array_size(expr) if( expr && ASR::is_a<ASR::ArraySize_t>(*expr) ) { \
                    ASR::ArraySize_t* array_size_t = ASR::down_cast<ASR::ArraySize_t>(expr); \
                    if( ASRUtils::is_pointer(ASRUtils::expr_type(array_size_t->m_v)) ) { \
                        return true; \
                    } \
                } \

                check_pointer_in_array_size(m_dims[i].m_start)
                check_pointer_in_array_size(m_dims[i].m_length)

            }

            return false;
        }

        void visit_Allocate(const ASR::Allocate_t& x) {
            for( size_t i = 0; i < x.n_args; i++ ) {
                ASR::alloc_arg_t alloc_arg = x.m_args[i];
                if( !ASRUtils::is_dimension_dependent_only_on_arguments(
                        alloc_arg.m_dims, alloc_arg.n_dims) ||
                    is_array_size_called_on_pointer(alloc_arg.m_dims, alloc_arg.n_dims) ) {
                    if( ASR::is_a<ASR::Var_t>(*alloc_arg.m_a) ) {
                        scope2var[current_scope].push_back(
                                ASR::down_cast<ASR::Var_t>(alloc_arg.m_a)->m_v);
                    }
                }
            }
        }

};

class PromoteAllocatableToNonAllocatable:
    public ASR::CallReplacerOnExpressionsVisitor<PromoteAllocatableToNonAllocatable>
{
    private:

        Allocator& al;
        bool remove_original_statement;

    public:

        std::map<SymbolTable*, std::vector<ASR::symbol_t*>>& scope2var;

        PromoteAllocatableToNonAllocatable(Allocator& al_,
            std::map<SymbolTable*, std::vector<ASR::symbol_t*>>& scope2var_):
            al(al_), remove_original_statement(false), scope2var(scope2var_) {}

        void visit_Allocate(const ASR::Allocate_t& x) {
            ASR::Allocate_t& xx = const_cast<ASR::Allocate_t&>(x);
            Vec<ASR::alloc_arg_t> x_args;
            x_args.reserve(al, x.n_args);
            for( size_t i = 0; i < x.n_args; i++ ) {
                ASR::alloc_arg_t alloc_arg = x.m_args[i];
                if( ASR::is_a<ASR::Var_t>(*alloc_arg.m_a) &&
                    ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(alloc_arg.m_a)) &&
                    ASRUtils::is_array(ASRUtils::expr_type(alloc_arg.m_a)) &&
                    ASR::is_a<ASR::Variable_t>(
                        *ASR::down_cast<ASR::Var_t>(alloc_arg.m_a)->m_v) &&
                    ASRUtils::expr_intent(alloc_arg.m_a) == ASRUtils::intent_local &&
                    ASRUtils::is_dimension_dependent_only_on_arguments(
                        alloc_arg.m_dims, alloc_arg.n_dims) &&
                    std::find(scope2var[current_scope].begin(),
                        scope2var[current_scope].end(),
                        ASR::down_cast<ASR::Var_t>(alloc_arg.m_a)->m_v) ==
                        scope2var[current_scope].end() ) {
                    ASR::Variable_t* alloc_variable = ASR::down_cast<ASR::Variable_t>(
                        ASR::down_cast<ASR::Var_t>(alloc_arg.m_a)->m_v);
                    alloc_variable->m_type = ASRUtils::make_Array_t_util(al, x.base.base.loc,
                        ASRUtils::type_get_past_array(
                            ASRUtils::type_get_past_allocatable(alloc_variable->m_type)),
                        alloc_arg.m_dims, alloc_arg.n_dims);
                } else if( ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(alloc_arg.m_a)) ||
                           ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(alloc_arg.m_a)) ) {
                    x_args.push_back(al, alloc_arg);
                }
            }
            if( x_args.size() > 0 ) {
                xx.m_args = x_args.p;
                xx.n_args = x_args.size();
            } else {
                remove_original_statement = true;
            }
        }

        template <typename T>
        void visit_Deallocate(const T& x) {
            T& xx = const_cast<T&>(x);
            Vec<ASR::expr_t*> x_args;
            x_args.reserve(al, x.n_vars);
            for( size_t i = 0; i < x.n_vars; i++ ) {
                if( ASR::is_a<ASR::Allocatable_t>(
                        *ASRUtils::expr_type(x.m_vars[i])) ||
                    ASR::is_a<ASR::Pointer_t>(
                        *ASRUtils::expr_type(x.m_vars[i])) ) {
                    x_args.push_back(al, x.m_vars[i]);
                }
            }
            if( x_args.size() > 0 ) {
                xx.m_vars = x_args.p;
                xx.n_vars = x_args.size();
            } else {
                remove_original_statement = true;
            }
        }

        void visit_ExplicitDeallocate(const ASR::ExplicitDeallocate_t& x) {
            visit_Deallocate(x);
        }

        void visit_ImplicitDeallocate(const ASR::ImplicitDeallocate_t& x) {
            visit_Deallocate(x);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            bool remove_original_statement_copy = remove_original_statement;
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            for (size_t i = 0; i < n_body; i++) {
                remove_original_statement = false;
                visit_stmt(*m_body[i]);
                if( !remove_original_statement ) {
                    body.push_back(al, m_body[i]);
                }
            }
            m_body = body.p;
            n_body = body.size();
            remove_original_statement = remove_original_statement_copy;
        }

};

class FixArrayPhysicalCast: public ASR::BaseExprReplacer<FixArrayPhysicalCast> {
    private:
        Allocator& al;

    public:

        FixArrayPhysicalCast(Allocator& al_): al(al_) {}

        void replace_ArrayPhysicalCast(ASR::ArrayPhysicalCast_t* x) {
            ASR::BaseExprReplacer<FixArrayPhysicalCast>::replace_ArrayPhysicalCast(x);
            if( x->m_old != ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg)) ) {
                x->m_old = ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_arg));
            }
            if( (x->m_old == x->m_new &&
                x->m_old != ASR::array_physical_typeType::DescriptorArray) ||
                (x->m_old == x->m_new && x->m_old == ASR::array_physical_typeType::DescriptorArray &&
                (ASR::is_a<ASR::Allocatable_t>(*ASRUtils::expr_type(x->m_arg)) ||
                ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(x->m_arg))) ) ) {
                *current_expr = x->m_arg;
            }
        }

        void replace_FunctionCall(ASR::FunctionCall_t* x) {
            ASR::BaseExprReplacer<FixArrayPhysicalCast>::replace_FunctionCall(x);
            ASR::expr_t* call = ASRUtils::EXPR(ASRUtils::make_FunctionCall_t_util(
                al, x->base.base.loc, x->m_name, x->m_original_name, x->m_args,
                x->n_args, x->m_type, x->m_value, x->m_dt,
                ASRUtils::get_class_proc_nopass_val((*x).m_name)));
            ASR::FunctionCall_t* function_call = ASR::down_cast<ASR::FunctionCall_t>(call);
            x->m_args = function_call->m_args;
            x->n_args = function_call->n_args;
        }

        void replace_ArrayReshape(ASR::ArrayReshape_t* x) {
            ASR::BaseExprReplacer<FixArrayPhysicalCast>::replace_ArrayReshape(x);
            if( ASRUtils::extract_physical_type(ASRUtils::expr_type(x->m_array)) ==
                ASR::array_physical_typeType::FixedSizeArray &&
                ASRUtils::extract_physical_type(x->m_type) !=
                ASR::array_physical_typeType::FixedSizeArray ) {
                size_t n_dims = ASRUtils::extract_n_dims_from_ttype(x->m_type);
                Vec<ASR::dimension_t> empty_dims; empty_dims.reserve(al, n_dims);
                for( size_t i = 0; i < n_dims; i++ ) {
                    ASR::dimension_t empty_dim;
                    empty_dim.loc = x->base.base.loc;
                    empty_dim.m_start = nullptr;
                    empty_dim.m_length = nullptr;
                    empty_dims.push_back(al, empty_dim);
                }
                x->m_type = ASRUtils::TYPE(ASR::make_Array_t(al, x->base.base.loc,
                    ASRUtils::extract_type(x->m_type), empty_dims.p, empty_dims.size(),
                    ASR::array_physical_typeType::FixedSizeArray));
            }
        }
};

class FixArrayPhysicalCastVisitor: public ASR::CallReplacerOnExpressionsVisitor<FixArrayPhysicalCastVisitor> {
    public:

        Allocator& al;
        FixArrayPhysicalCast replacer;

        FixArrayPhysicalCastVisitor(Allocator& al_): al(al_), replacer(al_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::CallReplacerOnExpressionsVisitor<FixArrayPhysicalCastVisitor>::visit_SubroutineCall(x);
            ASR::stmt_t* call = ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(
                al, x.base.base.loc, x.m_name, x.m_original_name, x.m_args,
                x.n_args, x.m_dt, nullptr, false, ASRUtils::get_class_proc_nopass_val(x.m_name)));
            ASR::SubroutineCall_t* subrout_call = ASR::down_cast<ASR::SubroutineCall_t>(call);
            ASR::SubroutineCall_t& xx = const_cast<ASR::SubroutineCall_t&>(x);
            xx.m_args = subrout_call->m_args;
            xx.n_args = subrout_call->n_args;
        }
};

void pass_promote_allocatable_to_nonallocatable(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const PassOptions &/*pass_options*/) {
    std::map<SymbolTable*, std::vector<ASR::symbol_t*>> scope2var;
    IsAllocatedCalled is_allocated_called(scope2var);
    is_allocated_called.visit_TranslationUnit(unit);
    PromoteAllocatableToNonAllocatable promoter(al, scope2var);
    promoter.visit_TranslationUnit(unit);
    promoter.visit_TranslationUnit(unit);
    FixArrayPhysicalCastVisitor fix_array_physical_cast(al);
    fix_array_physical_cast.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}

} // namespace LCompilers
