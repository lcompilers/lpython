#ifndef LIBASR_PASS_INLINE_FUNCTION_CALLS_H
#define LIBASR_PASS_INLINE_FUNCTION_CALLS_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_inline_function_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                    const std::string& rl_path,
                                    bool inline_external_symbol_calls=true);

} // namespace LCompilers

#endif // LIBASR_PASS_FMA_H
