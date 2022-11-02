#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/global_stmts.h>
#include <libasr/pass/pass_utils.h>


namespace LFortran {

using ASR::down_cast;

using LFortran::ASRUtils::EXPR;

/*
 * This ASR pass transforms (in-place) the ASR tree and wras all global
 * statements and expressions into a function.
 *
 */
void pass_wrap_global_stmts_into_function(Allocator &al,
            ASR::TranslationUnit_t &unit, const LCompilers::PassOptions& pass_options) {
    std::string fn_name_s = pass_options.run_fun;
    if (unit.n_items > 0) {
        // Add an anonymous function
        Str s;
        s.from_str_view(fn_name_s);
        char *fn_name = s.c_str(al);
        SymbolTable *fn_scope = al.make_new<SymbolTable>(unit.m_global_scope);

        ASR::ttype_t *type;
        Location loc = unit.base.base.loc;
        ASR::asr_t *return_var=nullptr;
        ASR::expr_t *return_var_ref=nullptr;
        char *var_name;
        int idx = 1;

        Vec<ASR::stmt_t*> body;
        body.reserve(al, unit.n_items);
        for (size_t i=0; i<unit.n_items; i++) {
            if (unit.m_items[i]->type == ASR::asrType::expr) {
                ASR::expr_t *target;
                ASR::expr_t *value = EXPR(unit.m_items[i]);
                // Create a new variable with the right type
                if (LFortran::ASRUtils::expr_type(value)->type == ASR::ttypeType::Integer) {
                    s.from_str(al, fn_name_s + std::to_string(idx));
                    var_name = s.c_str(al);

                    int a_kind = down_cast<ASR::Integer_t>(LFortran::ASRUtils::expr_type(value))->m_kind;

                    type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, a_kind, nullptr, 0));
                    return_var = ASR::make_Variable_t(al, loc,
                        fn_scope, var_name, LFortran::ASRUtils::intent_local, nullptr, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::BindC,
                        ASR::Public, ASR::presenceType::Required, false);
                    return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                        down_cast<ASR::symbol_t>(return_var)));
                    fn_scope->add_symbol(std::string(var_name), down_cast<ASR::symbol_t>(return_var));
                    target = return_var_ref;
                    idx++;
                } else if (LFortran::ASRUtils::expr_type(value)->type == ASR::ttypeType::Real) {
                    s.from_str(al, fn_name_s + std::to_string(idx));
                    var_name = s.c_str(al);
                    type = ASRUtils::expr_type(value);
                    return_var = ASR::make_Variable_t(al, loc,
                        fn_scope, var_name, LFortran::ASRUtils::intent_local, nullptr, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::BindC,
                        ASR::Public, ASR::presenceType::Required, false);
                    return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                        down_cast<ASR::symbol_t>(return_var)));
                    fn_scope->add_symbol(std::string(var_name), down_cast<ASR::symbol_t>(return_var));
                    target = return_var_ref;
                    idx++;
                } else if (LFortran::ASRUtils::expr_type(value)->type == ASR::ttypeType::Complex) {
                    s.from_str(al, fn_name_s + std::to_string(idx));
                    var_name = s.c_str(al);
                    type = ASRUtils::expr_type(value);
                    return_var = ASR::make_Variable_t(al, loc,
                        fn_scope, var_name, LFortran::ASRUtils::intent_local, nullptr, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::BindC,
                        ASR::Public, ASR::presenceType::Required, false);
                    return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                        down_cast<ASR::symbol_t>(return_var)));
                    fn_scope->add_symbol(std::string(var_name), down_cast<ASR::symbol_t>(return_var));
                    target = return_var_ref;
                    idx++;
                } else {
                    throw LCompilersException("Return type not supported in interactive mode");
                }
                ASR::stmt_t* asr_stmt = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target, value, nullptr));
                body.push_back(al, asr_stmt);
            } else if (unit.m_items[i]->type == ASR::asrType::stmt) {
                ASR::stmt_t* asr_stmt = LFortran::ASRUtils::STMT(unit.m_items[i]);
                body.push_back(al, asr_stmt);
                return_var = nullptr;
            } else {
                throw LCompilersException("Unsupported type of global scope node");
            }
        }

        if (return_var) {
            // The last item was an expression, create a function returning it

            // The last defined `return_var` is the actual return value
            ASR::down_cast2<ASR::Variable_t>(return_var)->m_intent = LFortran::ASRUtils::intent_return_var;


            ASR::asr_t *fn = ASR::make_Function_t(
                al, loc,
                /* a_symtab */ fn_scope,
                /* a_name */ fn_name,
                nullptr, 0,
                /* a_args */ nullptr,
                /* n_args */ 0,
                /* a_body */ body.p,
                /* n_body */ body.size(),
                /* a_return_var */ return_var_ref,
                ASR::abiType::BindC,
                ASR::Public, ASR::Implementation,
                nullptr,
                false, false, false, false, false,
                nullptr, 0,
                nullptr, 0,
                false);
            std::string sym_name = fn_name;
            if (unit.m_global_scope->get_symbol(sym_name) != nullptr) {
                throw LCompilersException("Function already defined");
            }
            unit.m_global_scope->add_symbol(sym_name, down_cast<ASR::symbol_t>(fn));
        } else {
            // The last item was a statement, create a subroutine (returing
            // nothing)
            ASR::asr_t *fn = ASR::make_Function_t(
                al, loc,
                /* a_symtab */ fn_scope,
                /* a_name */ fn_name,
                nullptr, 0,
                /* a_args */ nullptr,
                /* n_args */ 0,
                /* a_body */ body.p,
                /* n_body */ body.size(),
                nullptr,
                ASR::abiType::Source,
                ASR::Public, ASR::Implementation, nullptr,
                false, false, false, false, false,
                nullptr, 0,
                nullptr, 0,
                false);
            std::string sym_name = fn_name;
            if (unit.m_global_scope->get_symbol(sym_name) != nullptr) {
                throw LCompilersException("Function already defined");
            }
            unit.m_global_scope->add_symbol(sym_name, down_cast<ASR::symbol_t>(fn));
        }
        unit.m_items = nullptr;
        unit.n_items = 0;
        PassUtils::UpdateDependenciesVisitor v(al);
        v.visit_TranslationUnit(unit);
        LFORTRAN_ASSERT(asr_verify(unit));
    }
}

} // namespace LFortran
