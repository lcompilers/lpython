#ifndef LFORTRAN_ASR_TO_WASM_H
#define LFORTRAN_ASR_TO_WASM_H

#include <libasr/asr.h>

namespace LCompilers {

// Generates a wasm binary stream from ASR
Result<Vec<uint8_t>> asr_to_wasm_bytes_stream(ASR::TranslationUnit_t &asr,
                                              Allocator &al,
                                              diag::Diagnostics &diagnostics,
                                              CompilerOptions &co);

// Generates a wasm binary to `filename`
Result<int> asr_to_wasm(ASR::TranslationUnit_t &asr, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics, CompilerOptions &co);

}  // namespace LCompilers

#endif  // LFORTRAN_ASR_TO_WASM_H
