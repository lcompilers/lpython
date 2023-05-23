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
    std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid;

    public:

    ReplaceIntrinsicFunction(Allocator& al_, SymbolTable* global_scope_,
    std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid_) :
        al(al_), global_scope(global_scope_), func2intrinsicid(func2intrinsicid_) {}


    void replace_IntrinsicFunction(ASR::IntrinsicFunction_t* x) {
        Vec<ASR::call_arg_t> new_args;
        // Replace any IntrinsicFunctions in the argument first:
        {
            new_args.reserve(al, x->n_args);
            for( size_t i = 0; i < x->n_args; i++ ) {
                ASR::expr_t** current_expr_copy_ = current_expr;
                current_expr = &(x->m_args[i]);
                replace_expr(x->m_args[i]);
                ASR::call_arg_t arg0;
                arg0.m_value = *current_expr; // Use the converted arg
                new_args.push_back(al, arg0);
                current_expr = current_expr_copy_;
            }
        }
        // TODO: currently we always instantiate a new function.
        // Rather we should reuse the old instantiation if it has
        // exactly the same arguments. For that we could use the
        // overload_id, and uniquely encode the argument types.
        // We could maintain a mapping of type -> id and look it up.

        ASRUtils::impl_function instantiate_function =
            ASRUtils::IntrinsicFunctionRegistry::get_instantiate_function(x->m_intrinsic_id);
        if( instantiate_function == nullptr ) {
            return ;
        }
        Vec<ASR::ttype_t*> arg_types;
        arg_types.reserve(al, x->n_args);
        for( size_t i = 0; i < x->n_args; i++ ) {
            arg_types.push_back(al, ASRUtils::expr_type(x->m_args[i]));
        }
        *current_expr = instantiate_function(al, x->base.base.loc,
            global_scope, arg_types, new_args, x->m_overload_id, x->m_value);
        ASR::expr_t* func_call = *current_expr;
        LCOMPILERS_ASSERT(ASR::is_a<ASR::FunctionCall_t>(*func_call));
        ASR::FunctionCall_t* function_call_t = ASR::down_cast<ASR::FunctionCall_t>(func_call);
        ASR::symbol_t* function_call_t_symbol = ASRUtils::symbol_get_past_external(function_call_t->m_name);
        func2intrinsicid[function_call_t_symbol] = (ASRUtils::IntrinsicFunctions) x->m_intrinsic_id;
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

        ReplaceIntrinsicFunctionVisitor(Allocator& al_, SymbolTable* global_scope_,
            std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid_) :
            replacer(al_, global_scope_, func2intrinsicid_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

class ReplaceFunctionCallReturningArray: public ASR::BaseExprReplacer<ReplaceFunctionCallReturningArray> {

    private:

    Allocator& al;
    Vec<ASR::stmt_t*>& pass_result;
    size_t result_counter;
    std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid;

    public:

    SymbolTable* current_scope;

    ReplaceFunctionCallReturningArray(Allocator& al_, Vec<ASR::stmt_t*>& pass_result_,
    std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid_) :
    al(al_), pass_result(pass_result_), result_counter(0),
    func2intrinsicid(func2intrinsicid_), current_scope(nullptr) {}

    // Not called from anywhere but kept for future use.
    // Especially if we don't find alternatives to allocatables
    ASR::expr_t* get_result_var_for_runtime_dim(ASR::expr_t* dim, int n_dims,
        std::string m_name, const Location& loc, ASR::ttype_t* m_type,
        ASR::expr_t* input_array) {
        ASR::expr_t* result_var_ = PassUtils::create_var(result_counter,
                                    m_name + "_res",
                                    loc, m_type, al, current_scope,
                                    ASR::storage_typeType::Allocatable);
        ASR::stmt_t** else_ = nullptr;
        size_t else_n = 0;
        const Location& loc_ = dim->base.loc;
        for( int i = 1; i <= n_dims + 1; i++ ) {
            ASR::expr_t* current_dim = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                        al, loc_, i, ASRUtils::expr_type(dim)));
            ASR::expr_t* test_expr = ASRUtils::EXPR(ASR::make_IntegerCompare_t(
                                        al, loc_, dim, ASR::cmpopType::Eq,
                                        current_dim, ASRUtils::TYPE(ASR::make_Logical_t(
                                            al, loc_, 4, nullptr, 0)), nullptr));

            ASR::alloc_arg_t alloc_arg;
            alloc_arg.loc = loc_;
            alloc_arg.m_a = result_var_;
            Vec<ASR::dimension_t> alloc_dims;
            alloc_dims.reserve(al, n_dims);
            for( int j = 1; j <= n_dims + 1; j++ ) {
                if( j == i ) {
                    continue ;
                }
                ASR::dimension_t m_dim;
                m_dim.loc = loc_;
                m_dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                al, loc_, 1, ASRUtils::expr_type(dim)));
                // Assuming that first argument is the array
                m_dim.m_length = ASRUtils::get_size(input_array, j, al);
                alloc_dims.push_back(al, m_dim);
            }
            alloc_arg.m_dims = alloc_dims.p;
            alloc_arg.n_dims = alloc_dims.size();
            Vec<ASR::alloc_arg_t> alloc_args;
            alloc_args.reserve(al, 1);
            alloc_args.push_back(al, alloc_arg);
            ASR::stmt_t* allocate_stmt = ASRUtils::STMT(ASR::make_Allocate_t(
                al, loc_, alloc_args.p, alloc_args.size(), nullptr, nullptr, nullptr));
            Vec<ASR::stmt_t*> if_body;
            if_body.reserve(al, 1);
            if_body.push_back(al, allocate_stmt);
            ASR::stmt_t* if_ = ASRUtils::STMT(ASR::make_If_t(al, loc_, test_expr,
                                if_body.p, if_body.size(), else_, else_n));
            Vec<ASR::stmt_t*> if_else_if;
            if_else_if.reserve(al, 1);
            if_else_if.push_back(al, if_);
            else_ = if_else_if.p;
            else_n = if_else_if.size();
        }
        pass_result.push_back(al, else_[0]);
        return result_var_;
    }

    ASR::expr_t* get_result_var_for_constant_dim(int dim, int n_dims,
        std::string m_name, const Location& loc, ASR::ttype_t* m_type,
        ASR::expr_t* input_array) {
        Vec<ASR::dimension_t> result_dims;
        result_dims.reserve(al, n_dims);
        for( int j = 1; j <= n_dims + 1; j++ ) {
            if( j == dim ) {
                continue ;
            }
            ASR::dimension_t m_dim;
            m_dim.loc = loc;
            m_dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, 1,
                            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0)) ));
            // Assuming that first argument is the array
            m_dim.m_length = ASRUtils::get_size(input_array, j, al);
            result_dims.push_back(al, m_dim);
        }
        ASR::ttype_t* result_type = ASRUtils::duplicate_type(al, m_type, &result_dims);
        return PassUtils::create_var(result_counter,
                                    m_name + "_res",
                                    loc, result_type, al, current_scope);
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x) {
        ASR::symbol_t* x_m_name = ASRUtils::symbol_get_past_external(x->m_name);
        int n_dims = ASRUtils::extract_n_dims_from_ttype(x->m_type);
        if( func2intrinsicid.find(x_m_name) == func2intrinsicid.end() ||
            n_dims == 0 ) {
            return ;
        }

        Vec<ASR::call_arg_t> new_args;
        new_args.reserve(al, x->n_args + 1);
        for( size_t i = 0; i < x->n_args; i++ ) {
            if (x->m_args[i].m_value != nullptr) {
                ASR::expr_t** current_expr_copy_9 = current_expr;
                current_expr = &(x->m_args[i].m_value);
                this->replace_expr(x->m_args[i].m_value);
                ASR::call_arg_t new_arg;
                new_arg.loc = x->m_args[i].loc;
                new_arg.m_value = *current_expr;
                new_args.push_back(al, new_arg);
                current_expr = current_expr_copy_9;
            }
        }
        ASR::expr_t* result_var_ = nullptr;
        int dim_index = ASRUtils::IntrinsicFunctionRegistry::get_dim_index(func2intrinsicid[x_m_name]);
        if( dim_index != -1 ) {
            ASR::expr_t* dim = x->m_args[dim_index].m_value;
            if( !ASRUtils::is_value_constant(ASRUtils::expr_value(dim)) ) {
                // Possibly can be replaced by calling "get_result_var_for_runtime_dim"
                throw LCompilersException("Runtime values for dim argument is not supported yet.");
            } else {
                int constant_dim;
                ASRUtils::extract_value(ASRUtils::expr_value(dim), constant_dim);
                result_var_ = get_result_var_for_constant_dim(constant_dim, n_dims,
                                ASRUtils::symbol_name(x->m_name), x->base.base.loc,
                                x->m_type, x->m_args[0].m_value);
            }
        }
        result_counter += 1;
        ASR::call_arg_t new_arg;
        LCOMPILERS_ASSERT(result_var_)
        new_arg.loc = result_var_->base.loc;
        new_arg.m_value = result_var_;
        new_args.push_back(al, new_arg);
        pass_result.push_back(al, ASRUtils::STMT(ASR::make_SubroutineCall_t(
            al, x->base.base.loc, x->m_name, x->m_original_name, new_args.p,
            new_args.size(), x->m_dt)));

        *current_expr = result_var_;
    }

};

class ReplaceFunctionCallReturningArrayVisitor : public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallReturningArrayVisitor>
{
    private:

        Allocator& al;
        ReplaceFunctionCallReturningArray replacer;
        Vec<ASR::stmt_t*> pass_result;
        Vec<ASR::stmt_t*>* parent_body;

    public:

        ReplaceFunctionCallReturningArrayVisitor(Allocator& al_,
            std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions>& func2intrinsicid_) :
        al(al_), replacer(al_, pass_result, func2intrinsicid_), parent_body(nullptr) {
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

};

void pass_replace_intrinsic_function(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& /*pass_options*/) {
    std::map<ASR::symbol_t*, ASRUtils::IntrinsicFunctions> func2intrinsicid;
    ReplaceIntrinsicFunctionVisitor v(al, unit.m_global_scope, func2intrinsicid);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallReturningArrayVisitor u(al, func2intrinsicid);
    u.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
