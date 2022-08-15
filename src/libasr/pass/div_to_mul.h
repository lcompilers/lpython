#ifndef LIBASR_PASS_DIV_TO_MUL_H
#define LIBASR_PASS_DIV_TO_MUL_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_div_to_mul(Allocator &al, ASR::TranslationUnit_t &unit,
                                 const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LIBASR_PASS_DIV_TO_MUL_H
