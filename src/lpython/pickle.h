#ifndef LCOMPILERS_PICKLE_H
#define LCOMPILERS_PICKLE_H

#include <libasr/asr.h>

namespace LCompilers {

    // Pickle an ASR node
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

    // Print the tree structure
    std::string pickle_tree(LCompilers::ASR::asr_t &asr, bool colors, bool show_intrinsic_modules);
    std::string pickle_tree(LCompilers::ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules);

}

#endif // LCOMPILERS_PICKLE_H
