#ifndef LCOMPILERS_ASR_TO_X86_H
#define LCOMPILERS_ASR_TO_X86_H

#include <libasr/asr.h>

namespace LCompilers {

    // Generates a 32-bit x86 Linux executable binary `filename`
    Result<int> asr_to_x86(ASR::TranslationUnit_t &asr, Allocator &al,
            const std::string &filename, bool time_report);

} // namespace LCompilers

#endif // LCOMPILERS_ASR_TO_X86_H
