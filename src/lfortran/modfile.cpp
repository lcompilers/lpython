#include <string>

#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/modfile.h>
#include <lfortran/serialization.h>


namespace LFortran {


class BinaryWriter
{
private:
    std::string s;
public:
    std::string get_str() {
        return s;
    }

    std::string uint64_to_string(uint64_t i) {
        char bytes[4];
        bytes[0] = (i >> 24) & 0xFF;
        bytes[1] = (i >> 16) & 0xFF;
        bytes[2] = (i >>  8) & 0xFF;
        bytes[3] =  i        & 0xFF;
        return std::string(bytes, 4);
    }

    uint64_t string_to_uint64(const char *p) {
        return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    }

    uint64_t string_to_uint64(const std::string &s) {
        return string_to_uint64(&s[0]);
    }

    void write_int8(uint8_t i) {
        char c=i;
        s.append(std::string(&c, 1));
    }

    void write_int64(uint64_t i) {
        s.append(uint64_to_string(i));
    }

    void write_bool(bool b) {
        if (b) {
            write_int8(1);
        } else {
            write_int8(0);
        }
    }

    void write_string(const std::string &t) {
        write_int64(t.size());
        s.append(t);
    }
};

class BinaryReader
{
private:
    std::string s;
    size_t pos;
public:
    BinaryReader(const std::string &s) : s{s}, pos{0} {}

    uint64_t string_to_uint64(const char *p) {
        return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    }

    uint64_t string_to_uint64(const std::string &s) {
        return string_to_uint64(&s[0]);
    }

    uint8_t read_int8() {
        if (pos+1 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint8_t n = s[pos];
        pos += 1;
        return n;
    }

    uint64_t read_int64() {
        if (pos+4 > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        uint64_t n = string_to_uint64(&s[pos]);
        pos += 4;
        return n;
    }

    bool read_bool() {
        uint8_t b = read_int8();
        return (b == 1);
    }

    std::string read_string() {
        size_t n = read_int64();
        if (pos+n > s.size()) {
            throw LFortranException("String is too short for deserialization.");
        }
        std::string r = std::string(&s[pos], n);
        pos += n;
        return r;
    }
};

const std::string lfortran_modfile_type_string = "LFortran Modfile";

// The save_modfile() and load_modfile() must stay consistent. What is saved
// must be loaded in exactly the same order.

/*
    Saves the module into a binary stream.

    That stream can be saved to a mod file by the caller.
    The sections in the file/stream are saved using write_string(), so they
    can be efficiently read by the loader and ignored if needed.

    Comments below show some possible future improvements to the mod format.
*/
std::string save_modfile(const ASR::TranslationUnit_t &m) {
    LFORTRAN_ASSERT(m.m_global_scope->scope.size()== 1);
    for (auto &a : m.m_global_scope->scope) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
    }
    BinaryWriter b;
    // Header
    b.write_string(lfortran_modfile_type_string);
    b.write_string(LFORTRAN_VERSION);

    // AST section: Original module source code:
    // Currently empty.
    // Note: in the future we can save here:
    // * A path to the original source code
    // * Hash of the orig source code
    // * AST binary export of it (this AST only changes if the hash changes)

    // ASR section:

    // Export ASR:
    // Currently empty.

    // Full ASR:
    b.write_string(serialize(m));

    return b.get_str();
}

ASR::TranslationUnit_t* load_modfile(Allocator &al, const std::string &s) {
    BinaryReader b(s);
    std::string file_type = b.read_string();
    if (file_type != lfortran_modfile_type_string) {
        throw LFortranException("LFortran Modfile format not recognized");
    }
    std::string version = b.read_string();
    if (version != LFORTRAN_VERSION) {
        throw LFortranException("Incompatible format: LFortran Modfile was generated using version '" + version + "', but current LFortran version is '" + LFORTRAN_VERSION + "'");
    }
    std::string asr_binary = b.read_string();
    ASR::asr_t *asr = deserialize_asr(al, asr_binary);
    return ASR::down_cast2<ASR::TranslationUnit_t>(asr);
}

} // namespace LFortran
