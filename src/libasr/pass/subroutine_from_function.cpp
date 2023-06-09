#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/subroutine_from_function.h>
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
            for (auto &item : x.m_global_scope->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    PassUtils::handle_fn_return_var(al,
                        ASR::down_cast<ASR::Function_t>(item.second),
                        PassUtils::is_aggregate_type);
                }
            }

            std::vector<std::string> build_order
                = ASRUtils::determine_module_dependencies(x);
            for (auto &item : build_order) {
                LCOMPILERS_ASSERT(x.m_global_scope->get_symbol(item));
                ASR::symbol_t *mod = x.m_global_scope->get_symbol(item);
                visit_symbol(*mod);
            }
            // Now visit everything else
            for (auto &item : x.m_global_scope->get_scope()) {
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
                        PassUtils::is_aggregate_type);
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
                        PassUtils::is_aggregate_type);
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

    public:

        ASR::expr_t *result_var;
        SymbolTable* current_scope;

        ReplaceFunctionCallWithSubroutineCall(Allocator& al_,
            Vec<ASR::stmt_t*>& pass_result_):
        al(al_), result_counter(0),
        pass_result(pass_result_), result_var(nullptr)
        {}

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

            bool is_return_var_handled = false;
            ASR::symbol_t *fn_name = ASRUtils::symbol_get_past_external(x->m_name);
            if (ASR::is_a<ASR::Function_t>(*fn_name)) {
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
                is_return_var_handled = fn->m_return_var == nullptr;
            }
            if (!is_return_var_handled) {
                return ;
            }

            if( result_var == nullptr || !ASRUtils::is_array(x->m_type) ) {
                result_var = PassUtils::create_var(result_counter,
                    "_libasr_created_return_var_",
                    x->base.base.loc, x->m_type, al, current_scope);
                result_counter += 1;
            }
            LCOMPILERS_ASSERT(result_var != nullptr);
            *current_expr = result_var;

            Vec<ASR::call_arg_t> s_args;
            s_args.reserve(al, x->n_args + 1);
            for( size_t i = 0; i < x->n_args; i++ ) {
                s_args.push_back(al, x->m_args[i]);
            }
            ASR::call_arg_t result_arg;
            result_arg.loc = result_var->base.loc;
            result_arg.m_value = result_var;
            s_args.push_back(al, result_arg);
            ASR::stmt_t* subrout_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(
                al, x->base.base.loc, x->m_name, nullptr, s_args.p, s_args.size(), nullptr));
            pass_result.push_back(al, subrout_call);
            result_var = nullptr;
        }

};

class ReplaceFunctionCallWithSubroutineCallVisitor:
    public ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor> {

    private:

        Allocator& al;
        Vec<ASR::stmt_t*> pass_result;
        ReplaceFunctionCallWithSubroutineCall replacer;
        Vec<ASR::stmt_t*>* parent_body;

    public:

        ReplaceFunctionCallWithSubroutineCallVisitor(Allocator& al_):
         al(al_), replacer(al, pass_result),
         parent_body(nullptr)
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

        void visit_Assignment(const ASR::Assignment_t& x) {
            if( PassUtils::is_aggregate_type(x.m_target) ) {
                replacer.result_var = x.m_target;
            }
            ASR::CallReplacerOnExpressionsVisitor<ReplaceFunctionCallWithSubroutineCallVisitor>::visit_Assignment(x);
            replacer.result_var = nullptr;
        }

};

void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                          const LCompilers::PassOptions& /*pass_options*/) {
    CreateFunctionFromSubroutine v(al);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallWithSubroutineCallVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
