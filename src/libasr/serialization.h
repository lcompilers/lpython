#ifndef LIBASR_SERIALIZATION_H
#define LIBASR_SERIALIZATION_H

#include <libasr/asr.h>

namespace LCompilers {

    std::string serialize(const ASR::asr_t &asr);
    std::string serialize(const ASR::TranslationUnit_t &unit);
    ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s,
            bool load_symtab_id, SymbolTable &symtab);
    ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s,
            bool load_symtab_id);

    void fix_external_symbols(ASR::TranslationUnit_t &unit,
            SymbolTable &external_symtab);
} // namespace LCompilers

#endif // LIBASR_SERIALIZATION_H
