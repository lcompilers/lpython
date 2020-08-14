#ifndef LFORTRAN_ASR_TO_CPP_H
#define LFORTRAN_ASR_TO_CPP_H

#include <lfortran/asr.h>

namespace LFortran {

    std::string asr_to_cpp(ASR::TranslationUnit_t &asr);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_CPP_H
