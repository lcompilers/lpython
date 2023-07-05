#ifndef LIBASR_PASS_FLIP_SIGN_H
#define LIBASR_PASS_FLIP_SIGN_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_flip_sign(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_FLIP_SIGN_H
