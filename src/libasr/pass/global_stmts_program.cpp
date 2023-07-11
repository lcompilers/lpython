#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/wrap_global_stmts.h>
#include <libasr/pass/wrap_global_symbols.h>


namespace LCompilers {

using ASR::down_cast;

using ASRUtils::EXPR;

/*
 * This ASR pass transforms (in-place) the ASR tree and wraps all global
 * statements and expressions into a program.
 *
 */
void pass_wrap_global_stmts_program(Allocator &al,
            ASR::TranslationUnit_t &unit, const LCompilers::PassOptions& pass_options) {
    std::string program_fn_name = pass_options.run_fun;
    SymbolTable *current_scope = al.make_new<SymbolTable>(unit.m_global_scope);
    std::string prog_name = "main_program";
    Vec<ASR::stmt_t*> prog_body;
    prog_body.reserve(al, 1);
    SetChar prog_dep;
    prog_dep.reserve(al, 1);
    bool call_main_program = unit.n_items > 0;
    pass_wrap_global_stmts(al, unit, pass_options);
    pass_wrap_global_symbols(al, unit, pass_options);
    if( call_main_program && !pass_options.disable_main ) {
        // Call `_lpython_main_program` function
        ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(
            unit.m_global_scope->get_symbol("_global_symbols"));
        ASR::symbol_t *fn_s = mod->m_symtab->get_symbol(program_fn_name);
        if (!(ASR::is_a<ASR::Function_t>(*fn_s)
                && ASR::down_cast<ASR::Function_t>(fn_s)->m_return_var == nullptr)) {
            throw LCompilersException("Return type not supported yet");
        }

        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(fn_s);
        fn_s = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
            al, fn->base.base.loc, current_scope, s2c(al, program_fn_name),
            fn_s, mod->m_name, nullptr, 0, s2c(al, program_fn_name),
            ASR::accessType::Public));
        current_scope->add_symbol(program_fn_name, fn_s);
        ASR::asr_t *stmt = ASRUtils::make_SubroutineCall_t_util(
                al, unit.base.base.loc,
                fn_s, nullptr,
                nullptr, 0,
                nullptr,
                nullptr, false);
        prog_body.push_back(al, ASR::down_cast<ASR::stmt_t>(stmt));
        prog_dep.push_back(al, s2c(al, "_global_symbols"));
    }

    if( !pass_options.disable_main ) {
        ASR::asr_t *prog = ASR::make_Program_t(
            al, unit.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ s2c(al, prog_name),
            prog_dep.p,
            prog_dep.n,
            /* a_body */ prog_body.p,
            /* n_body */ prog_body.n);
        unit.m_global_scope->add_symbol(prog_name, ASR::down_cast<ASR::symbol_t>(prog));
    }
}

} // namespace LCompilers
