#ifndef LFORTRAN_ASR_TO_JULIA_H
#define LFORTRAN_ASR_TO_JULIA_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/codegen/asr_to_c_cpp.h>
// #include <julia.h>

namespace LFortran
{
Result<std::string>
asr_to_julia(Allocator& al, ASR::TranslationUnit_t& asr, diag::Diagnostics& diag);
}  // namespace LFortran

#endif  // LFORTRAN_ASR_TO_JULIA_H
