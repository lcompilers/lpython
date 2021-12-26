#ifndef LFORTRAN_PASS_IMPLIED_DO_LOOPS_H
#define LFORTRAN_PASS_IMPLIED_DO_LOOPS_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_replace_implied_do_loops(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_IMPLIED_DO_LOOPS_H
