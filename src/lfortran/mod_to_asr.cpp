#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <map>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/mod_to_asr.h>


namespace LFortran {

using ASR::down_cast;
using ASR::down_cast2;

ASR::TranslationUnit_t *mod_to_asr(Allocator &al, std::string filename)
{
    std::ifstream in(filename, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(in), {});

    SymbolTable *symtab = al.make_new<SymbolTable>(nullptr);
    ASR::asr_t *asr;
    Location loc;
    asr = ASR::make_TranslationUnit_t(al, loc,
        symtab, nullptr, 0);
    return down_cast2<ASR::TranslationUnit_t>(asr);
}

} // namespace LFortran
