#ifndef LIBASR_PASS_UNUSED_FUNCTIONS_H
#define LIBASR_PASS_UNUSED_FUNCTIONS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_unused_functions(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_UNUSED_FUNCTIONS_H
