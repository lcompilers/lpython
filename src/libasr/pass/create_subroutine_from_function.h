#ifndef LIBASR_PASS_CREATE_SUBROUTINE_FROM_FUNCTION_H
#define LIBASR_PASS_CREATE_SUBROUTINE_FROM_FUNCTION_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_CREATE_SUBROUTINE_FROM_FUNCTION_H
