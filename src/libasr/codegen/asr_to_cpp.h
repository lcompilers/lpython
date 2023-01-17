#ifndef LFORTRAN_ASR_TO_CPP_H
#define LFORTRAN_ASR_TO_CPP_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    Result<std::string> asr_to_cpp(Allocator &al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics, Platform &platform, int64_t default_lower_bound);

} // namespace LCompilers

#endif // LFORTRAN_ASR_TO_CPP_H
