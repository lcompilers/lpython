#ifndef LFORTRAN_ASR_TO_X86_H
#define LFORTRAN_ASR_TO_X86_H

#include <lfortran/asr.h>

namespace LFortran {

    // Generates a 32-bit x86 Linux executable binary `filename`
    void asr_to_x86(ASR::TranslationUnit_t &asr, Allocator &al,
            const std::string &filename, bool time_report);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_X86_H
