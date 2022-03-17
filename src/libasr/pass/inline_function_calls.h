#ifndef LIBASR_PASS_INLINE_FUNCTION_CALLS_H
#define LIBASR_PASS_INLINE_FUNCTION_CALLS_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_inline_function_calls(Allocator &al, ASR::TranslationUnit_t &unit, const std::string& rl_path);

} // namespace LFortran

#endif // LIBASR_PASS_FMA_H
