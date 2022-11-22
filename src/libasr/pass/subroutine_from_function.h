#ifndef LIBASR_PASS_SUBROUTINE_FROM_FUNCTION_H
#define LIBASR_PASS_SUBROUTINE_FROM_FUNCTION_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_create_subroutine_from_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                              const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LIBASR_PASS_SUBROUTINE_FROM_FUNCTION_H
