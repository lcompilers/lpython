#ifndef LIBASR_PICKLE_H
#define LIBASR_PICKLE_H

#include <libasr/asr.h>
#include <libasr/location.h>

namespace LCompilers {

    // Pickle an ASR node
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

    // Print the tree structure
    std::string pickle_tree(ASR::asr_t &asr, bool colors, bool show_intrinsic_modules=false);
    std::string pickle_tree(ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules=false);

    // Print Json structure
    std::string pickle_json(ASR::asr_t &asr, LocationManager &lm, bool show_intrinsic_modules=false);
    std::string pickle_json(ASR::TranslationUnit_t &asr, LocationManager &lm, bool show_intrinsic_modules=false);

} // namespace LCompilers

#endif // LIBASR_PICKLE_H
