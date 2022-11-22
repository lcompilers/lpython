#ifndef LIBASR_ASR_TO_CPP_H
#define LIBASR_ASR_TO_CPP_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    Result<std::string> asr_to_cpp(Allocator &al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics, CompilerOptions &co,
        int64_t default_lower_bound);

} // namespace LCompilers

#endif // LIBASR_ASR_TO_CPP_H
