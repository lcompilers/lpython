#ifndef LIBASR_PASS_DIV_TO_MUL_H
#define LIBASR_PASS_DIV_TO_MUL_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_div_to_mul(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_DIV_TO_MUL_H
