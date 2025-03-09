#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/create_subroutine_from_function.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>
#include <libasr/pass/pass_utils.h>

namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

class CreateFunctionFromSubroutine: public ASR::BaseWalkVisitor<CreateFunctionFromSubroutine> {

    public:

        Allocator& al;

        CreateFunctionFromSubroutine(Allocator &al_): al(al_)
        {
        }

        void visit_Function(const ASR::Function_t& x) {
            ASR::Function_t& xx = const_cast<ASR::Function_t&>(x);
            ASR::Function_t* x_ptr = ASR::down_cast<ASR::Function_t>(&(xx.base));
            PassUtils::handle_fn_return_var(al, x_ptr, PassUtils::is_aggregate_or_array_or_nonPrimitive_type);
        }

};

class ReplaceFunctionCallWithSubroutineCall:
    public ASR::BaseExprReplacer<ReplaceFunctionCallWithSubroutineCall> {
private :
public :
    Allocator & al;
    int result_counter = 0;
    SymbolTable* current_scope;
    Vec<ASR::stmt_t*> &pass_result;
    ReplaceFunctionCallWithSubroutineCall(Allocator& al_, Vec<ASR::stmt_t*> &pass_result_) :
        al(al_),pass_result(pass_result_) {}

    void traverse_functionCall_args(ASR::call_arg_t* call_args, size_t call_args_n){
        for(size_t i = 0; i < call_args_n; i++){
            ASR::expr_t** current_expr_copy = current_expr;
            current_expr = &call_args[i].m_value;
            replace_expr(call_args[i].m_value);
            current_expr = current_expr_copy;
        }
    }

    void replace_FunctionCall(ASR::FunctionCall_t* x){
        traverse_functionCall_args(x->m_args, x->n_args);
        if(PassUtils::is_non_primitive_return_type(x->m_type)){ // Arrays and structs are handled by the array_struct_temporary. No need to check for them here.
            // Create variable in current_scope to be holding the return + Deallocate.
            ASR::expr_t* result_var = PassUtils::create_var(result_counter++,
                "_func_call_res", x->base.base.loc, ASRUtils::duplicate_type(al, x->m_type), al, current_scope);
            if(ASRUtils::is_allocatable(result_var)){
                Vec<ASR::expr_t*> to_be_deallocated;
                to_be_deallocated.reserve(al, 1);
                to_be_deallocated.push_back(al, result_var);
                pass_result.push_back(al, ASRUtils::STMT(
                ASR::make_ImplicitDeallocate_t(al, result_var->base.loc,
                    to_be_deallocated.p, to_be_deallocated.size())));
            }
            // Create new call args with `result_var` as last argument capturing return + Create a `subroutineCall`.
            Vec<ASR::call_arg_t> new_call_args;
            new_call_args.reserve(al,1);
            new_call_args.from_pointer_n_copy(al, x->m_args, x->n_args);
            new_call_args.push_back(al, {result_var->base.loc, result_var});
            ASR::stmt_t* subrout_call = ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, x->base.base.loc,
                                                x->m_name, nullptr, new_call_args.p, new_call_args.size(), x->m_dt,
                                                nullptr, false, false));
            // replace functionCall with `result_var` + push subroutineCall into the body.
            *current_expr = result_var;
            pass_result.push_back(al, subrout_call);
        }
    }
};
class ReplaceFunctionCallWithSubroutineCallVisitor:
    public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor> {

    private:

        Allocator& al;
        Vec<ASR::stmt_t*> pass_result;
        ReplaceFunctionCallWithSubroutineCall replacer;
        bool remove_original_statement = false;
        Vec<ASR::stmt_t*>* parent_body = nullptr;


    public:

        ReplaceFunctionCallWithSubroutineCallVisitor(Allocator& al_): al(al_), replacer(al, pass_result)
        {
            pass_result.n = 0;
            pass_result.reserve(al, 1);
        }

        void call_replacer(){
            replacer.current_expr = current_expr;
            replacer.current_scope = current_scope;
            replacer.replace_expr(*current_expr);
        }

        void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
            Vec<ASR::stmt_t*> body;
            body.reserve(al, n_body);
            if(!pass_result.empty()){  // Flush `pass_result`.
                LCOMPILERS_ASSERT(parent_body != nullptr);
                for(size_t i = 0; i < pass_result.size(); i++){
                    parent_body->push_back(al, pass_result[i]);
                }
                pass_result.n = 0;
            }
            bool remove_original_statement_copy = remove_original_statement;
            for (size_t i = 0; i < n_body; i++) {
                parent_body = &body;
                remove_original_statement = false;
                visit_stmt(*m_body[i]);
                if( pass_result.size() > 0 ) {
                    for (size_t j=0; j < pass_result.size(); j++) {
                        body.push_back(al, pass_result[j]);
                    }
                    pass_result.n = 0;
                }
                if (!remove_original_statement){
                    body.push_back(al, m_body[i]);
                }
            }
            remove_original_statement = remove_original_statement_copy;
            m_body = body.p;
            n_body = body.size();
        }

        bool is_function_call_returning_aggregate_type(ASR::expr_t* m_value) {
            bool is_function_call = ASR::is_a<ASR::FunctionCall_t>(*m_value);
            bool is_aggregate_type = (ASRUtils::is_aggregate_type(
                ASRUtils::expr_type(m_value)) ||
                PassUtils::is_aggregate_or_array_type(m_value));
            return is_function_call && is_aggregate_type;
        }

        void visit_Assignment(const ASR::Assignment_t &x) {
            ASR::CallReplacerOnExpressionsVisitor \
            <ReplaceFunctionCallWithSubroutineCallVisitor>::visit_Assignment(x);
            if( !is_function_call_returning_aggregate_type(x.m_value)) {
                return ;
            }

            ASR::FunctionCall_t* fc = ASR::down_cast<ASR::FunctionCall_t>(x.m_value);
            if( PassUtils::is_elemental(fc->m_name) && ASRUtils::is_array(fc->m_type) ) {
                return ;
            }
            const Location& loc = x.base.base.loc;
            Vec<ASR::call_arg_t> s_args;
            s_args.reserve(al, fc->n_args + 1);
            for( size_t i = 0; i < fc->n_args; i++ ) {
                s_args.push_back(al, fc->m_args[i]);
            }
            if(ASRUtils::is_allocatable(x.m_value) &&
               ASRUtils::is_allocatable(x.m_target)){ // Make sure to deallocate the argument that will hold the return of function.
                Vec<ASR::expr_t*> to_be_deallocated;
                to_be_deallocated.reserve(al, 1);
                to_be_deallocated.push_back(al, x.m_target);
                pass_result.push_back(al, ASRUtils::STMT(
                    ASR::make_ImplicitDeallocate_t(al, x.m_target->base.loc,
                    to_be_deallocated.p, to_be_deallocated.size())));
            }
            ASR::call_arg_t result_arg;
            result_arg.loc = x.m_target->base.loc;
            result_arg.m_value = x.m_target;
            s_args.push_back(al, result_arg);
            ASR::stmt_t* subrout_call = ASRUtils::STMT(ASRUtils::make_SubroutineCall_t_util(al, loc,
                fc->m_name, fc->m_original_name, s_args.p, s_args.size(), fc->m_dt, nullptr, false, false));
            pass_result.push_back(al, subrout_call);
            remove_original_statement = true;
        }
};

void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                          const LCompilers::PassOptions& /*pass_options*/) {
    CreateFunctionFromSubroutine v(al);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallWithSubroutineCallVisitor u(al);
    u.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor w(al);
    w.visit_TranslationUnit(unit);
}


} // namespace LCompilers
