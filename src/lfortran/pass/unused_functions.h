#ifndef LFORTRAN_PASS_UNUSED_FUNCTIONS_H
#define LFORTRAN_PASS_UNUSED_FUNCTIONS_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_unused_functions(ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_UNUSED_FUNCTIONS_H
