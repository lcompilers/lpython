#ifndef LIBASR_PASS_LOOP_UNROLL_H
#define LIBASR_PASS_LOOP_UNROLL_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_loop_unroll(Allocator &al, ASR::TranslationUnit_t &unit,
                          const std::string& rl_path, int64_t unroll_factor=32);

} // namespace LFortran

#endif // LIBASR_PASS_LOOP_UNROLL_H
