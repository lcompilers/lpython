#include <string>

#include <libasr/config.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/modfile.h>
#include <libasr/serialization.h>
#include <libasr/bwriter.h>

namespace LCompilers {

const std::string lfortran_modfile_type_string = "LCompilers Modfile";

inline void save_asr(const ASR::TranslationUnit_t &m, std::string& asr_string, LCompilers::LocationManager lm) {
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

    // Full LocationManager:
    b.write_int32(lm.files.size());
    for(auto file: lm.files) {
        // std::vector<FileLocations> files;
        b.write_string(file.in_filename);
        b.write_int32(file.current_line);

        // std::vector<uint32_t> out_start
        b.write_int32(file.out_start.size());
        for(auto i: file.out_start) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_start
        b.write_int32(file.in_start.size());
        for(auto i: file.in_start) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_newlines
        b.write_int32(file.in_newlines.size());
        for(auto i: file.in_newlines) {
            b.write_int32(i);
        }

        // bool preprocessor
        b.write_int32(file.preprocessor);

        // std::vector<uint32_t> out_start0
        b.write_int32(file.out_start0.size());
        for(auto i: file.out_start0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_start0
        b.write_int32(file.in_start0.size());
        for(auto i: file.in_start0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_size0
        b.write_int32(file.in_size0.size());
        for(auto i: file.in_size0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> interval_type0
        b.write_int32(file.interval_type0.size());
        for(auto i: file.interval_type0) {
            b.write_int32(i);
        }

        // std::vector<uint32_t> in_newlines0
        b.write_int32(file.in_newlines0.size());
        for(auto i: file.in_newlines0) {
            b.write_int32(i);
        }
    }

    // std::vector<uint32_t> file_ends
    b.write_int32(lm.file_ends.size());
    for(auto i: lm.file_ends) {
        b.write_int32(i);
    }

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
std::string save_modfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm) {
    LCOMPILERS_ASSERT(m.m_symtab->get_scope().size()== 1);
    for (auto &a : m.m_symtab->get_scope()) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        if ((bool&)a) { } // Suppress unused warning in Release mode
    }

    std::string asr_string;
    save_asr(m, asr_string, lm);
    return asr_string;
}

std::string save_pycfile(const ASR::TranslationUnit_t &m, LCompilers::LocationManager lm) {
    std::string asr_string;
    save_asr(m, asr_string, lm);
    return asr_string;
}

inline void load_serialised_asr(const std::string &s, std::string& asr_binary,
                                LCompilers::LocationManager &lm) {
#ifdef WITH_LFORTRAN_BINARY_MODFILES
    BinaryReader b(s);
#else
    TextReader b(s);
#endif
    std::string file_type = b.read_string();
    if (file_type != lfortran_modfile_type_string) {
        throw LCompilersException("LCompilers Modfile format not recognized");
    }
    std::string version = b.read_string();
    if (version != LFORTRAN_VERSION) {
        throw LCompilersException("Incompatible format: LFortran Modfile was generated using version '" + version + "', but current LFortran version is '" + LFORTRAN_VERSION + "'");
    }
    LCompilers::LocationManager serialized_lm;
    int32_t n_files = b.read_int32();
    std::vector<LCompilers::LocationManager::FileLocations> files;
    for(int i=0; i<n_files; i++) {
        LCompilers::LocationManager::FileLocations file;
        file.in_filename = b.read_string();
        file.current_line = b.read_int32();

        int32_t n_out_start = b.read_int32();
        for(int i=0; i<n_out_start; i++) {
            file.out_start.push_back(b.read_int32());
        }

        int32_t n_in_start = b.read_int32();
        for(int i=0; i<n_in_start; i++) {
            file.in_start.push_back(b.read_int32());
        }

        int32_t n_in_newlines = b.read_int32();
        for(int i=0; i<n_in_newlines; i++) {
            file.in_newlines.push_back(b.read_int32());
        }

        file.preprocessor = b.read_int32();

        int32_t n_out_start0 = b.read_int32();
        for(int i=0; i<n_out_start0; i++) {
            file.out_start0.push_back(b.read_int32());
        }

        int32_t n_in_start0 = b.read_int32();
        for(int i=0; i<n_in_start0; i++) {
            file.in_start0.push_back(b.read_int32());
        }

        int32_t n_in_size0 = b.read_int32();
        for(int i=0; i<n_in_size0; i++) {
            file.in_size0.push_back(b.read_int32());
        }

        int32_t n_interval_type0 = b.read_int32();
        for(int i=0; i<n_interval_type0; i++) {
            file.interval_type0.push_back(b.read_int32());
        }

        int32_t n_in_newlines0 = b.read_int32();
        for(int i=0; i<n_in_newlines0; i++) {
            file.in_newlines0.push_back(b.read_int32());
        }

        serialized_lm.files.push_back(file);
    }

    int32_t n_file_ends = b.read_int32();
    for(int i=0; i<n_file_ends; i++) {
        serialized_lm.file_ends.push_back(b.read_int32());
    }

    lm.files.push_back(serialized_lm.files[0]);
    lm.file_ends.push_back(serialized_lm.file_ends[0] + lm.file_ends.back());

    asr_binary = b.read_string();
}

ASR::TranslationUnit_t* load_modfile(Allocator &al, const std::string &s,
        bool load_symtab_id, SymbolTable &symtab, LCompilers::LocationManager &lm) {
    std::string asr_binary;
    load_serialised_asr(s, asr_binary, lm);
    // take offset as last second element of file_ends
    uint32_t offset = lm.file_ends[lm.file_ends.size()-2];
    ASR::asr_t *asr = deserialize_asr(al, asr_binary, load_symtab_id, symtab, offset);
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

ASR::TranslationUnit_t* load_pycfile(Allocator &al, const std::string &s,
        bool load_symtab_id, LCompilers::LocationManager &lm) {
    std::string asr_binary;
    load_serialised_asr(s, asr_binary, lm);
    uint32_t offset = 0;
    ASR::asr_t *asr = deserialize_asr(al, asr_binary, load_symtab_id, offset);

    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
    return tu;
}

} // namespace LCompilers
