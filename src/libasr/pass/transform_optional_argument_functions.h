#ifndef LFORTRAN_PASS_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS
#define LFORTRAN_PASS_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_transform_optional_argument_functions(
        Allocator &al, ASR::TranslationUnit_t &unit,
        const LCompilers::PassOptions& pass_options
    );

} // namespace LFortran

#endif // LFORTRAN_TRANSFORM_OPTIONAL_ARGUMENT_FUNCTIONS
