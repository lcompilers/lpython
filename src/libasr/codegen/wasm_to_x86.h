#ifndef LFORTRAN_WASM_TO_X86_H
#define LFORTRAN_WASM_TO_X86_H

#include <libasr/wasm_visitor.h>

namespace LFortran {

Result<int> wasm_to_x86(Vec<uint8_t> &wasm_bytes, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics);

}  // namespace LFortran

#endif  // LFORTRAN_WASM_TO_X86_H
