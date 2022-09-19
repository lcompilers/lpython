#ifndef LFORTRAN_PICKLE_H
#define LFORTRAN_PICKLE_H

#include <libasr/asr.h>

namespace LFortran {

    // Pickle an ASR node
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

    // Print the tree structure
    std::string pickle_tree(LFortran::ASR::asr_t &asr, bool colors, bool show_intrinsic_modules);
    std::string pickle_tree(LFortran::ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules);
    void write_asr_to_file(std::string file, LFortran::ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules);

}

#endif // LFORTRAN_PICKLE_H
