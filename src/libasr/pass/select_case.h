#ifndef LFORTRAN_PASS_SELECT_CASE_H
#define LFORTRAN_PASS_SELECT_CASE_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_SELECT_CASE_H
