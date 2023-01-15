#ifndef LIBASR_PASS_DEAD_CODE_REMOVAL_H
#define LIBASR_PASS_DEAD_CODE_REMOVAL_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_dead_code_removal(Allocator &al, ASR::TranslationUnit_t &unit,
                                const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif
