#ifndef LFORTRAN_MODFILE_H
#define LFORTRAN_MODFILE_H

#include <libasr/asr.h>

namespace LCompilers {

    // Save a module to a modfile
    std::string save_modfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm);

    std::string save_pycfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm);

    // Load a module from a modfile
    ASR::TranslationUnit_t* load_modfile(Allocator &al, const std::string &s,
        bool load_symtab_id, SymbolTable &symtab, LCompilers::LocationManager &lm);

    ASR::TranslationUnit_t* load_pycfile(Allocator &al, const std::string &s,
        bool load_symtab_id, LCompilers::LocationManager &lm);

} // namespace LCompilers

#endif // LFORTRAN_MODFILE_H
