#ifndef LIBASR_PASS_SIGN_FROM_VALUE_H
#define LIBASR_PASS_SIGN_FROM_VALUE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_sign_from_value(Allocator &al, ASR::TranslationUnit_t &unit,
                                      const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LIBASR_PASS_SIGN_FROM_VALUE_H
