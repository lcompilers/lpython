#ifndef LIBASR_PASS_REPLACE_ARR_SLICE_H
#define LIBASR_PASS_REPLACE_ARR_SLICE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_ARR_SLICE_H
