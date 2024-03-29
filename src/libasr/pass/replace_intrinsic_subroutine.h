#ifndef LIBASR_PASS_REPLACE_INTRINSIC_SUBROUTINE_H
#define LIBASR_PASS_REPLACE_INTRINSIC_SUBROUTINE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_intrinsic_subroutine(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_INTRINSIC_SUBROUTINE_H
