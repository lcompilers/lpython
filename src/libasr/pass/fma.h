#ifndef LIBASR_PASS_FMA_H
#define LIBASR_PASS_FMA_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_fma(Allocator &al, ASR::TranslationUnit_t &unit,
                          const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LIBASR_PASS_FMA_H
