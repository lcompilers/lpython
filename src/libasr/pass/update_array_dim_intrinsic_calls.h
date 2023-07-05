#ifndef LIBASR_PASS_UPDATE_ARRAY_DIM_INTRINSIC_CALLS_H
#define LIBASR_PASS_UPDATE_ARRAY_DIM_INTRINSIC_CALLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_update_array_dim_intrinsic_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_UPDATE_ARRAY_DIM_INTRINSIC_CALLS_H
