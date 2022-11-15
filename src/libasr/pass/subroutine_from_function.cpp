#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/subroutine_from_function.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <string>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

class CreateFunctionFromSubroutine: public PassUtils::PassVisitor<CreateFunctionFromSubroutine> {

    public:

        CreateFunctionFromSubroutine(Allocator &al_) :
        PassVisitor(al_, nullptr)
        {
            pass_result.reserve(al, 1);
        }

        ASR::symbol_t* create_subroutine_from_function(ASR::Function_t* s) {
            for( auto& s_item: s->m_symtab->get_scope() ) {
                ASR::symbol_t* curr_sym = s_item.second;
                if( curr_sym->type == ASR::symbolType::Variable ) {
                    ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(curr_sym);
                    if( var->m_intent == ASR::intentType::Unspecified ) {
                        var->m_intent = ASR::intentType::In;
                    } else if( var->m_intent == ASR::intentType::ReturnVar ) {
                        var->m_intent = ASR::intentType::Out;
                    }
                }
            }
            Vec<ASR::expr_t*> a_args;
            a_args.reserve(al, s->n_args + 1);
            for( size_t i = 0; i < s->n_args; i++ ) {
                a_args.push_back(al, s->m_args[i]);
            }
            LFORTRAN_ASSERT(s->m_return_var)
            a_args.push_back(al, s->m_return_var);
            ASR::asr_t* s_sub_asr = ASR::make_Function_t(al, s->base.base.loc,
                s->m_symtab,
                s->m_name, s->m_dependencies, s->n_dependencies,
                a_args.p, a_args.size(), s->m_body, s->n_body,
                nullptr,
                s->m_abi, s->m_access, s->m_deftype, nullptr, false, false,
                false, s->m_inline, s->m_static,
                s->m_type_params, s->n_type_params,
                s->m_restrictions, s->n_restrictions,
                s->m_is_restriction);
            ASR::symbol_t* s_sub = ASR::down_cast<ASR::symbol_t>(s_sub_asr);
            return s_sub;
        }

        void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
            std::vector<std::pair<std::string, ASR::symbol_t*>> replace_vec;
            // Transform functions returning arrays to subroutines
            for (auto &item : x.m_global_scope->get_scope()) {
                if (is_a<ASR::Function_t>(*item.second)) {
                    ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                    if (s->m_return_var) {
                        /*
                        * A function which returns a aggregate type like array, struct will be converted
                        * to a subroutine with the destination array as the last
                        * argument. This helps in avoiding deep copies and the
                        * destination memory directly gets filled inside the subroutine.
                        */
                        if( PassUtils::is_aggregate_type(s->m_return_var) ) {
                            ASR::symbol_t* s_sub = create_subroutine_from_function(s);
                            replace_vec.push_back(std::make_pair(item.first, s_sub));
                        }
                    }
                }
            }

            // FIXME: this is a hack, we need to pass in a non-const `x`,
            // which requires to generate a TransformVisitor.
            ASR::TranslationUnit_t &xx = const_cast<ASR::TranslationUnit_t&>(x);
            // Updating the symbol table so that now the name
            // of the function (which returned array) now points
            // to the newly created subroutine.
            for( auto& item: replace_vec ) {
                xx.m_global_scope->add_symbol(item.first, item.second);
            }

            // Now visit everything else
            for (auto &item : x.m_global_scope->get_scope()) {
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
                    ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                    if (s->m_return_var) {
                        /*
                        * A function which returns an array will be converted
                        * to a subroutine with the destination array as the last
                        * argument. This helps in avoiding deep copies and the
                        * destination memory directly gets filled inside the subroutine.
                        */
                        if( PassUtils::is_aggregate_type(s->m_return_var) ) {
                            ASR::symbol_t* s_sub = create_subroutine_from_function(s);
                            replace_vec.push_back(std::make_pair(item.first, s_sub));
                        }
                    }
                }
            }

            // Updating the symbol table so that now the name
            // of the function (which returned array) now points
            // to the newly created subroutine.
            for( auto& item: replace_vec ) {
                current_scope->add_symbol(item.first, item.second);
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
            std::string x_name;
            if( x.m_name->type == ASR::symbolType::ExternalSymbol ) {
                x_name = down_cast<ASR::ExternalSymbol_t>(x.m_name)->m_name;
            } else if( x.m_name->type == ASR::symbolType::Function ) {
                x_name = down_cast<ASR::Function_t>(x.m_name)->m_name;
            }
            // The following checks if the name of a function actually
            // points to a subroutine. If true this would mean that the
            // original function returned an array and is now a subroutine.
            // So the current function call will be converted to a subroutine
            // call. In short, this check acts as a signal whether to convert
            // a function call to a subroutine call.
            if (current_scope == nullptr) {
                return ;
            }

            ASR::symbol_t *sub = current_scope->resolve_symbol(x_name);
            if (sub && ASR::is_a<ASR::Function_t>(*sub)
                && ASR::down_cast<ASR::Function_t>(sub)->m_return_var == nullptr) {
                LFORTRAN_ASSERT(result_var != nullptr);
                Vec<ASR::call_arg_t> s_args;
                s_args.reserve(al, x.n_args + 1);
                for( size_t i = 0; i < x.n_args; i++ ) {
                    s_args.push_back(al, x.m_args[i]);
                }
                ASR::call_arg_t result_arg;
                result_arg.loc = result_var->base.loc;
                result_arg.m_value = result_var;
                s_args.push_back(al, result_arg);
                ASR::stmt_t* subrout_call = LFortran::ASRUtils::STMT(ASR::make_SubroutineCall_t(al, x.base.base.loc,
                                                    sub, nullptr,
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
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
