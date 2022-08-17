#ifndef LIBASR_PASS_INLINE_FUNCTION_CALLS_H
#define LIBASR_PASS_INLINE_FUNCTION_CALLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_inline_function_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                    const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LIBASR_PASS_FMA_H
