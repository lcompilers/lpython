#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/global_stmts.h>


namespace LFortran {

using ASR::down_cast;

/*
 * This ASR pass transforms (in-place) the ASR tree and wras all global
 * statements and expressions into a function.
 *
 */
void pass_wrap_global_stmts_into_function(Allocator &al,
            ASR::TranslationUnit_t &unit, const std::string &fn_name_s) {
    if (unit.n_items > 0) {
        // Add an anonymous function
        Str s;
        s.from_str_view(fn_name_s);
        char *fn_name = s.c_str(al);
        SymbolTable *fn_scope = al.make_new<SymbolTable>(unit.m_global_scope);

        ASR::ttype_t *type;
        Location loc;
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
                if (expr_type(value)->type == ASR::ttypeType::Integer) {
                    s.from_str(al, fn_name_s + std::to_string(idx));
                    var_name = s.c_str(al);

                    int a_kind = down_cast<ASR::Integer_t>(expr_type(value))->m_kind;

                    type = TYPE(ASR::make_Integer_t(al, loc, a_kind, nullptr, 0));
                    return_var = ASR::make_Variable_t(al, loc,
                        fn_scope, var_name, intent_local, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public);
                    return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                        down_cast<ASR::symbol_t>(return_var)));
                    fn_scope->scope[std::string(var_name)] = down_cast<ASR::symbol_t>(return_var);
                    target = return_var_ref;
                    idx++;
                } else if (expr_type(value)->type == ASR::ttypeType::Real) {
                    s.from_str(al, fn_name_s + std::to_string(idx));
                    var_name = s.c_str(al);
                    type = TYPE(ASR::make_Real_t(al, loc, 4, nullptr, 0));
                    return_var = ASR::make_Variable_t(al, loc,
                        fn_scope, var_name, intent_local, nullptr,
                        ASR::storage_typeType::Default, type,
                        ASR::abiType::Source,
                        ASR::Public);
                    return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                        down_cast<ASR::symbol_t>(return_var)));
                    fn_scope->scope[std::string(var_name)] = down_cast<ASR::symbol_t>(return_var);
                    target = return_var_ref;
                    idx++;
                } else {
                    throw SemanticError("Return type not supported in interactive mode",
                            loc);
                }
                ASR::stmt_t* asr_stmt = STMT(ASR::make_Assignment_t(al, loc, target, value));
                body.push_back(al, asr_stmt);
            } else if (unit.m_items[i]->type == ASR::asrType::stmt) {
                ASR::stmt_t* asr_stmt = STMT(unit.m_items[i]);
                body.push_back(al, asr_stmt);
                return_var = nullptr;
            } else {
                throw CodeGenError("Unsupported type of global scope node");
            }
        }

        if (return_var) {
            // The last item was an expression, create a function returning it

            // The last defined `return_var` is the actual return value
            ASR::down_cast2<ASR::Variable_t>(return_var)->m_intent = intent_return_var;

            ASR::asr_t *fn = ASR::make_Function_t(
                al, loc,
                /* a_symtab */ fn_scope,
                /* a_name */ fn_name,
                /* a_args */ nullptr,
                /* n_args */ 0,
                /* a_body */ body.p,
                /* n_body */ body.size(),
                /* a_return_var */ return_var_ref,
                ASR::abiType::Source,
                ASR::Public, ASR::Implementation);
            std::string sym_name = fn_name;
            if (unit.m_global_scope->scope.find(sym_name) != unit.m_global_scope->scope.end()) {
                throw SemanticError("Function already defined", fn->loc);
            }
            unit.m_global_scope->scope[sym_name] = down_cast<ASR::symbol_t>(fn);
        } else {
            // The last item was a statement, create a subroutine (returing
            // nothing)
            ASR::asr_t *fn = ASR::make_Subroutine_t(
                al, loc,
                /* a_symtab */ fn_scope,
                /* a_name */ fn_name,
                /* a_args */ nullptr,
                /* n_args */ 0,
                /* a_body */ body.p,
                /* n_body */ body.size(),
                ASR::abiType::Source,
                ASR::Public, ASR::Implementation);
            std::string sym_name = fn_name;
            if (unit.m_global_scope->scope.find(sym_name) != unit.m_global_scope->scope.end()) {
                throw SemanticError("Function already defined", fn->loc);
            }
            unit.m_global_scope->scope[sym_name] = down_cast<ASR::symbol_t>(fn);
        }
        unit.m_items = nullptr;
        unit.n_items = 0;
        LFORTRAN_ASSERT(asr_verify(unit));
    }
}

} // namespace LFortran
