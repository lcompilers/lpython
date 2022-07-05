#ifndef LIBASR_PASS_LOOP_VECTORISE_H
#define LIBASR_PASS_LOOP_VECTORISE_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_loop_vectorise(Allocator &al, ASR::TranslationUnit_t &unit,
                             const std::string& rl_path);

} // namespace LFortran

#endif // LIBASR_PASS_LOOP_VECTORISE_H
