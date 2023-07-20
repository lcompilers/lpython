#ifndef LIBASR_PASS_LOOP_VECTORISE_H
#define LIBASR_PASS_LOOP_VECTORISE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_loop_vectorise(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_LOOP_VECTORISE_H
