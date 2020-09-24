#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <map>
#include <memory>

#include <zlib.h>

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
    // TODO: check the type of the module file here
    // For now we assume GFortran 14 or 15, which is ZLIB compressed
    std::vector<uint8_t> data(1024*1024);
    /*
    uint64_t data_size = data.size();
    std::cout << "Compressed data size: " << buffer.size() << std::endl;
    int res = uncompress(&data[0], &data_size, &buffer[0], buffer.size());
    switch(res) {
        case Z_OK:
            break;
        case Z_MEM_ERROR:
            throw LFortranException("ZLIB: out of memory");
        case Z_BUF_ERROR:
            throw LFortranException("ZLIB: output buffer was not large enough");
        case Z_DATA_ERROR:
            throw LFortranException("ZLIB: the input data was corrupted or incomplete");
        default:
            throw LFortranException("ZLIB: unknown error (" + std::to_string(res) + ")");
    }
    std::cout << "Uncompressed data size: " << data_size << std::endl;
    */

    SymbolTable *symtab = al.make_new<SymbolTable>(nullptr);
    ASR::asr_t *asr;
    Location loc;
    asr = ASR::make_TranslationUnit_t(al, loc,
        symtab, nullptr, 0);
    return down_cast2<ASR::TranslationUnit_t>(asr);
}

} // namespace LFortran
