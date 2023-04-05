#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/intrinsic_function.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

/*

This ASR pass replaces the IntrinsicFunction node with a call to an
implementation in ASR that we construct (and cache) on the fly for the actual
arguments.

Call this pass if you do not want to implement intrinsic functions directly
in the backend.

*/

class ReplaceIntrinsicFunction: public ASR::BaseExprReplacer<ReplaceIntrinsicFunction> {

    private:

    Allocator& al;
    SymbolTable* global_scope;

    public:

    ReplaceIntrinsicFunction(Allocator& al_, SymbolTable* global_scope_) :
        al(al_), global_scope(global_scope_) {}


    void replace_IntrinsicFunction(ASR::IntrinsicFunction_t* x) {
        LCOMPILERS_ASSERT(x->n_args == 1)
        Vec<ASR::call_arg_t> new_args;
        // Replace any IntrinsicFunctions in the argument first:
        {
            ASR::expr_t** current_expr_copy_ = current_expr;
            current_expr = &(x->m_args[0]);
            replace_expr(x->m_args[0]);
            new_args.reserve(al, x->n_args);
            ASR::call_arg_t arg0;
            arg0.m_value = *current_expr; // Use the converted arg
            new_args.push_back(al, arg0);
            current_expr = current_expr_copy_;
        }
        // TODO: currently we always instantiate a new function.
        // Rather we should reuse the old instantiation if it has
        // exactly the same arguments. For that we could use the
        // overload_id, and uniquely encode the argument types.
        // We could maintain a mapping of type -> id and look it up.
        if( !ASRUtils::IntrinsicFunctionRegistry::is_intrinsic_function(x->m_intrinsic_id) ) {
            throw LCompilersException("Intrinsic function not implemented");
        }

        ASRUtils::impl_function instantiate_function =
            ASRUtils::IntrinsicFunctionRegistry::get_instantiate_function(x->m_intrinsic_id);
        Vec<ASR::ttype_t*> arg_types;
        arg_types.reserve(al, x->n_args);
        for( size_t i = 0; i < x->n_args; i++ ) {
            arg_types.push_back(al, ASRUtils::expr_type(x->m_args[i]));
        }
        *current_expr = instantiate_function(al, x->base.base.loc,
            global_scope, arg_types, new_args, x->m_value);
    }

};

/*
The following visitor calls the above replacer i.e., ReplaceFunctionCalls
on expressions present in ASR so that FunctionCall get replaced everywhere
and we don't end up with false positives.
*/
class ReplaceIntrinsicFunctionVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceIntrinsicFunctionVisitor>
{
    private:

        ReplaceIntrinsicFunction replacer;

    public:

        ReplaceIntrinsicFunctionVisitor(Allocator& al_, SymbolTable* global_scope_) :
            replacer(al_, global_scope_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

void pass_replace_intrinsic_function(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& /*pass_options*/) {
    ReplaceIntrinsicFunctionVisitor v(al, unit.m_global_scope);
    v.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
