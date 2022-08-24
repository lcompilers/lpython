#include <string>

#include <libasr/config.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/modfile.h>
#include <libasr/serialization.h>
#include <libasr/bwriter.h>


namespace LFortran {

const std::string lfortran_modfile_type_string = "LFortran Modfile";

inline void save_asr(const ASR::TranslationUnit_t &m, std::string& asr_string) {
    #ifdef WITH_LFORTRAN_BINARY_MODFILES
    BinaryWriter b;
#else
    TextWriter b;
#endif
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

    asr_string = b.get_str();
}

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
    LFORTRAN_ASSERT(m.m_global_scope->get_scope().size()== 1);
    for (auto &a : m.m_global_scope->get_scope()) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        if ((bool&)a) { } // Suppress unused warning in Release mode
    }

    std::string asr_string;
    save_asr(m, asr_string);
    return asr_string;
}

std::string save_pycfile(const ASR::TranslationUnit_t &m) {
    std::string asr_string;
    save_asr(m, asr_string);
    return asr_string;
}

inline void load_serialised_asr(const std::string &s, std::string& asr_binary) {
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    BinaryReader b(s);
#else
    TextReader b(s);
#endif
    std::string file_type = b.read_string();
    if (file_type != lfortran_modfile_type_string) {
        throw LCompilersException("LFortran Modfile format not recognized");
    }
    std::string version = b.read_string();
    if (version != LFORTRAN_VERSION) {
        throw LCompilersException("Incompatible format: LFortran Modfile was generated using version '" + version + "', but current LFortran version is '" + LFORTRAN_VERSION + "'");
    }
    asr_binary = b.read_string();
}

ASR::TranslationUnit_t* load_modfile(Allocator &al, const std::string &s,
        bool load_symtab_id, SymbolTable &symtab) {
    std::string asr_binary;
    load_serialised_asr(s, asr_binary);
    ASR::asr_t *asr = deserialize_asr(al, asr_binary, load_symtab_id, symtab);

    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

ASR::TranslationUnit_t* load_pycfile(Allocator &al, const std::string &s,
        bool load_symtab_id) {
    std::string asr_binary;
    load_serialised_asr(s, asr_binary);
    ASR::asr_t *asr = deserialize_asr(al, asr_binary, load_symtab_id);

    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

} // namespace LFortran
