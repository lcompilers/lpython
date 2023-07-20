#ifndef LIBASR_PASS_INLINE_FUNCTION_CALLS_H
#define LIBASR_PASS_INLINE_FUNCTION_CALLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_inline_function_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_INLINE_FUNCTION_CALLS_H
