#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/wrap_global_stmts.h>
#include <libasr/pass/pass_utils.h>


namespace LCompilers {

using ASR::down_cast;

using ASRUtils::EXPR;

/*
 * This ASR pass transforms (in-place) the ASR tree and wras all global
 * statements and expressions into a function.
 *
 */
void pass_wrap_global_stmts(Allocator &al,
            ASR::TranslationUnit_t &unit, const LCompilers::PassOptions& pass_options) {
    if (unit.n_items == 0) {
        return ;
    }

    std::string fn_name_s = pass_options.run_fun;
    // Add an anonymous function
    Str s;
    s.from_str_view(fn_name_s);
    char *fn_name = s.c_str(al);
    SymbolTable *fn_scope = al.make_new<SymbolTable>(unit.m_symtab);

    ASR::ttype_t *type = nullptr;
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
            if ((ASRUtils::expr_type(value)->type == ASR::ttypeType::Integer) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::UnsignedInteger) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::Logical) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::Real) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::Complex) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::Character) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::List) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::Tuple) ||
                (ASRUtils::expr_type(value)->type == ASR::ttypeType::StructType)) {
                s.from_str(al, fn_name_s + std::to_string(idx));
                var_name = s.c_str(al);
                type = ASRUtils::expr_type(value);
                return_var = ASR::make_Variable_t(al, loc,
                    fn_scope, var_name, nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                    ASR::storage_typeType::Default, type,
                    nullptr, ASR::abiType::BindC,
                    ASR::Public, ASR::presenceType::Required, false);
                return_var_ref = EXPR(ASR::make_Var_t(al, loc,
                    down_cast<ASR::symbol_t>(return_var)));
                fn_scope->add_symbol(std::string(var_name), down_cast<ASR::symbol_t>(return_var));
                target = return_var_ref;
                idx++;
            }  else {
                throw LCompilersException("Return type not supported in interactive mode");
            }
            ASR::stmt_t* asr_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target, value, nullptr));
            body.push_back(al, asr_stmt);
        } else if (unit.m_items[i]->type == ASR::asrType::stmt) {
            ASR::stmt_t* asr_stmt = ASRUtils::STMT(unit.m_items[i]);
            body.push_back(al, asr_stmt);
            return_var = nullptr;
        } else {
            throw LCompilersException("Unsupported type of global scope node");
        }
    }

    if (return_var) {
        // The last defined `return_var` is the actual return value
        LCOMPILERS_ASSERT(type)
        LCOMPILERS_ASSERT(return_var_ref)

        ASR::down_cast2<ASR::Variable_t>(return_var)->m_intent = ASRUtils::intent_return_var;
        std::string global_underscore_name = "_" + fn_name_s;
        s.from_str(al, global_underscore_name);
        
        ASR::asr_t *global_underscore = ASR::make_Variable_t(al, loc,
                    unit.m_symtab, s.c_str(al), nullptr, 0, ASRUtils::intent_local, nullptr, nullptr,
                    ASR::storage_typeType::Default, type,
                    nullptr, ASR::abiType::Source,
                    ASR::Public, ASR::presenceType::Required, false);
        ASR::expr_t * global_underscore_ref = EXPR(ASR::make_Var_t(al, loc, down_cast<ASR::symbol_t>(global_underscore)));
        
        if (fn_scope->parent->get_symbol(global_underscore_name) != nullptr) {
            throw LCompilersException("Global variable already defined");
        }
        unit.m_symtab->add_symbol(global_underscore_name, down_cast<ASR::symbol_t>(global_underscore));
        ASR::stmt_t* asr_stmt = ASRUtils::STMT(ASR::make_Assignment_t(al, loc, global_underscore_ref, return_var_ref, nullptr));
        body.push_back(al, asr_stmt);

        if ((ASRUtils::expr_type(return_var_ref)->type == ASR::ttypeType::List) ||
            (ASRUtils::expr_type(return_var_ref)->type == ASR::ttypeType::Tuple) ||
            (ASRUtils::expr_type(return_var_ref)->type == ASR::ttypeType::StructType)) {
            return_var_ref = nullptr;
            return_var = nullptr;
        }
    }

    ASR::asr_t *fn = ASRUtils::make_Function_t_util(
        al, loc,
        /* a_symtab */ fn_scope,
        /* a_name */ fn_name,
        nullptr, 0,
        /* a_args */ nullptr,
        /* n_args */ 0,
        /* a_body */ body.p,
        /* n_body */ body.size(),
        /* a_return_var */ (return_var ? return_var_ref : nullptr),
        (return_var ? ASR::abiType::BindC : ASR::abiType::Source),
        ASR::Public, ASR::Implementation,
        nullptr,
        false, false, false, false, false,
        nullptr, 0,
        false, false, false);
    std::string sym_name = fn_name;
    if (unit.m_symtab->get_symbol(sym_name) != nullptr) {
        throw LCompilersException("Function already defined");
    }
    unit.m_symtab->add_symbol(sym_name, down_cast<ASR::symbol_t>(fn));
    unit.m_items = nullptr;
    unit.n_items = 0;
    PassUtils::UpdateDependenciesVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LCompilers
