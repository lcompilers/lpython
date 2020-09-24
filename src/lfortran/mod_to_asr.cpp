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

int uncompress_gzip(uint8_t *out, uint64_t *out_size, uint8_t *in,
        uint64_t in_size)
{
    // The code below is roughly equivalent to:
    //     return uncompress(out, out_size, in, in_size);
    // except that it enables gzip support in inflateInit2().
    int zlib_status;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = in;
    strm.avail_in = in_size;
    zlib_status = inflateInit2(&strm, 15 | 32);
    if (zlib_status < 0) return zlib_status;
    strm.next_out = out;
    strm.avail_out = *out_size;
    zlib_status = inflate(&strm, Z_NO_FLUSH);
    inflateEnd(&strm);
    *out_size = *out_size - strm.avail_out;
    return zlib_status;
}

ASR::TranslationUnit_t *mod_to_asr(Allocator &al, std::string filename)
{
    std::ifstream in(filename, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(in), {});
    // TODO: check the type of the module file here
    // For now we assume GFortran 14 or 15, which is ZLIB compressed
    std::vector<uint8_t> data(1024*1024);
    uint64_t data_size = data.size();
    std::cout << "Compressed data size: " << buffer.size() << std::endl;
    int res = uncompress_gzip(&data[0], &data_size, &buffer[0], buffer.size());
    switch(res) {
        case Z_OK:
        case Z_STREAM_END:
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

    SymbolTable *symtab = al.make_new<SymbolTable>(nullptr);
    ASR::asr_t *asr;
    Location loc;
    asr = ASR::make_TranslationUnit_t(al, loc,
        symtab, nullptr, 0);
    return down_cast2<ASR::TranslationUnit_t>(asr);
}

} // namespace LFortran
