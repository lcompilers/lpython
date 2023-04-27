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

class ReplaceFunctionCallWithSubroutineCall: public PassUtils::PassVisitor<ReplaceFunctionCallWithSubroutineCall> {

    private:

        ASR::expr_t *result_var;

    public:

        ReplaceFunctionCallWithSubroutineCall(Allocator& al_):
        PassVisitor(al_, nullptr), result_var(nullptr)
        {
            pass_result.reserve(al, 1);
        }

        void visit_Assignment(const ASR::Assignment_t& x) {
            if( PassUtils::is_aggregate_type(x.m_target) ) {
                result_var = x.m_target;
                this->visit_expr(*(x.m_value));
            }
            result_var = nullptr;
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
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
            ASR::symbol_t *fn_name = ASRUtils::symbol_get_past_external(x.m_name);
            if (ASR::is_a<ASR::Function_t>(*fn_name)) {
                ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_name);
                is_return_var_handled = fn->m_return_var == nullptr;
            }
            if (is_return_var_handled) {
                LCOMPILERS_ASSERT(result_var != nullptr);
                Vec<ASR::call_arg_t> s_args;
                s_args.reserve(al, x.n_args + 1);
                for( size_t i = 0; i < x.n_args; i++ ) {
                    s_args.push_back(al, x.m_args[i]);
                }
                ASR::call_arg_t result_arg;
                result_arg.loc = result_var->base.loc;
                result_arg.m_value = result_var;
                s_args.push_back(al, result_arg);
                ASR::stmt_t* subrout_call = ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc,
                                                    x.m_name, nullptr,
                                                    s_args.p, s_args.size(), nullptr));
                pass_result.push_back(al, subrout_call);
            }
            result_var = nullptr;
        }

};

void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                          const LCompilers::PassOptions& /*pass_options*/) {
    CreateFunctionFromSubroutine v(al);
    v.visit_TranslationUnit(unit);
    ReplaceFunctionCallWithSubroutineCall u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
