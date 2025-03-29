#ifndef LIBASR_PASS_PROMOTE_ALLOCATABLE_TO_NONALLOCATABLE_H
#define LIBASR_PASS_PROMOTE_ALLOCATABLE_TO_NONALLOCATABLE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_promote_allocatable_to_nonallocatable(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_PROMOTE_ALLOCATABLE_TO_NONALLOCATABLE_H
