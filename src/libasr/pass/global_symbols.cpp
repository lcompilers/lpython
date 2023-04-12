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
        const LCompilers::PassOptions &/*pass_options*/) {
    Location loc = unit.base.base.loc;
    char *module_name = s2c(al, "_global_symbols");
    SymbolTable *module_scope = al.make_new<SymbolTable>(unit.m_global_scope);
    Vec<char *> moved_symbols;
    SetChar mod_dependencies;

    // Move all the symbols from global into the module scope
    unit.m_global_scope->move_symbols_from_global_scope(al, module_scope,
        moved_symbols, mod_dependencies);

    // Erase the symbols that are moved into the module
    for (auto &sym: moved_symbols) {
        unit.m_global_scope->erase_symbol(sym);
    }

    ASR::symbol_t *module = (ASR::symbol_t *) ASR::make_Module_t(al, loc,
        module_scope, module_name, mod_dependencies.p, mod_dependencies.n,
        false, false);
    unit.m_global_scope->add_symbol(module_name, module);
}

} // namespace LCompilers
