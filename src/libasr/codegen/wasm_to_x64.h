#ifndef LFORTRAN_WASM_TO_X64_H
#define LFORTRAN_WASM_TO_X64_H

#include <libasr/wasm_visitor.h>

namespace LCompilers {

Result<int> wasm_to_x64(Vec<uint8_t> &wasm_bytes, Allocator &al,
                        const std::string &filename, bool time_report,
                        diag::Diagnostics &diagnostics);

}  // namespace LCompilers

#endif  // LFORTRAN_WASM_TO_X64_H
