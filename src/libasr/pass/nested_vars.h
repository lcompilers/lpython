#ifndef LIBASR_PASS_NESTED_VARS_H
#define LIBASR_PASS_NESTED_VARS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_nested_vars(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_NESTED_VARS_H
