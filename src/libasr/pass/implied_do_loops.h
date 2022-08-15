#ifndef LFORTRAN_PASS_IMPLIED_DO_LOOPS_H
#define LFORTRAN_PASS_IMPLIED_DO_LOOPS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_implied_do_loops(Allocator &al, ASR::TranslationUnit_t &unit,
                                       const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_IMPLIED_DO_LOOPS_H
