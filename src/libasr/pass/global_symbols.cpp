#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/global_symbols.h>
#include <libasr/pass/pass_utils.h>


namespace LCompilers {

/*
 * This ASR pass transforms (in-place) the ASR tree
 * and wraps all global symbols into a module
 */

void pass_wrap_global_syms_into_module(Allocator &al,
        ASR::TranslationUnit_t &unit,
        const LCompilers::PassOptions& pass_options) {
    Location loc = unit.base.base.loc;
    char *module_name = s2c(al, "_global_symbols");
    SymbolTable *module_scope = al.make_new<SymbolTable>(unit.m_global_scope);
    Vec<char *> moved_symbols;
    Vec<char *> mod_dependencies;
    Vec<char *> func_dependencies;
    Vec<ASR::stmt_t*> var_init;

    // Move all the symbols from global into the module scope
    unit.m_global_scope->move_symbols_from_global_scope(al, module_scope,
        moved_symbols, mod_dependencies, func_dependencies, var_init);

    // Erase the symbols that are moved into the module
    for (auto &sym: moved_symbols) {
        unit.m_global_scope->erase_symbol(sym);
    }

    if (module_scope->get_symbol(pass_options.run_fun) && var_init.n > 0) {
        ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(
            module_scope->get_symbol(pass_options.run_fun));
        for (size_t i = 0; i < f->n_body; i++) {
            var_init.push_back(al, f->m_body[i]);
        }
        for (size_t i = 0; i < f->n_dependencies; i++) {
            func_dependencies.push_back(al, f->m_dependencies[i]);
        }
        f->m_body = var_init.p;
        f->n_body = var_init.n;
        f->m_dependencies = func_dependencies.p;
        f->n_dependencies = func_dependencies.n;
        // Overwrites the function: `_lpython_main_program`
        module_scope->add_symbol(f->m_name, (ASR::symbol_t *) f);
    }

    ASR::symbol_t *module = (ASR::symbol_t *) ASR::make_Module_t(al, loc,
        module_scope, module_name, mod_dependencies.p, mod_dependencies.n,
        false, false);
    unit.m_global_scope->add_symbol(module_name, module);
}

} // namespace LCompilers
