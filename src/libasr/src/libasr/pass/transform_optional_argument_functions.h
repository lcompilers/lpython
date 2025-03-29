#ifndef LIBASR_PASS_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS_H
#define LIBASR_PASS_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_transform_optional_argument_functions(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS_H
