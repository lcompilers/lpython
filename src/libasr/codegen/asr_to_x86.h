#ifndef LIBASR_ASR_TO_X86_H
#define LIBASR_ASR_TO_X86_H

#include <libasr/asr.h>

namespace LCompilers {

    // Generates a 32-bit x86 Linux executable binary `filename`
    Result<int> asr_to_x86(ASR::TranslationUnit_t &asr, Allocator &al,
            const std::string &filename, bool time_report,
            diag::Diagnostics &diagnostics);

} // namespace LCompilers

#endif // LIBASR_ASR_TO_X86_H
