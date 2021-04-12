#ifndef LFORTRAN_PASS_PARAM_TO_CONST_H
#define LFORTRAN_PASS_PARAM_TO_CONST_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_replace_param_to_const(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_PARAM_TO_CONST_H
