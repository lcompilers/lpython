#ifndef LFORTRAN_PICKLE_H
#define LFORTRAN_PICKLE_H

#include <libasr/asr.h>

namespace LFortran {

    // Pickle an ASR node
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

}

#endif // LFORTRAN_PICKLE_H
