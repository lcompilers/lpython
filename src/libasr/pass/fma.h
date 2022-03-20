#ifndef LIBASR_PASS_FMA_H
#define LIBASR_PASS_FMA_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_replace_fma(Allocator &al, ASR::TranslationUnit_t &unit, const std::string& rl_path);

} // namespace LFortran

#endif // LIBASR_PASS_FMA_H
