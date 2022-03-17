#ifndef LIBASR_PASS_DIV_TO_MUL_H
#define LIBASR_PASS_DIV_TO_MUL_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_replace_div_to_mul(Allocator &al, ASR::TranslationUnit_t &unit, const std::string& rl_path);

} // namespace LFortran

#endif // LIBASR_PASS_DIV_TO_MUL_H
