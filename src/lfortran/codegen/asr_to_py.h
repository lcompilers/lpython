#ifndef LFORTRAN_ASR_TO_PY_H
#define LFORTRAN_ASR_TO_PY_H

#include <lfortran/asr.h>

namespace LFortran {

    std::string asr_to_py(ASR::TranslationUnit_t &asr);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_PY_H
