#ifndef LFORTRAN_ASR_TO_WASM_H
#define LFORTRAN_ASR_TO_WASM_H

#include <libasr/asr.h>

namespace LFortran {

    // Generates a 32-bit wasm Linux executable binary `filename`
    Result<int> asr_to_wasm(ASR::TranslationUnit_t &asr, Allocator &al,
            const std::string &filename, bool time_report);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_WASM_H
