#ifndef LIBASR_PASS_PARAM_TO_CONST_H
#define LIBASR_PASS_PARAM_TO_CONST_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_param_to_const(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_PARAM_TO_CONST_H
