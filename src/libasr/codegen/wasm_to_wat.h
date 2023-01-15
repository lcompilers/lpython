#ifndef LFORTRAN_WASM_TO_WAT_H
#define LFORTRAN_WASM_TO_WAT_H

#include <libasr/wasm_visitor.h>

namespace LCompilers {

Result<std::string> wasm_to_wat(Vec<uint8_t> &wasm_bytes, Allocator &al,
                                diag::Diagnostics &diagnostics);

}  // namespace LCompilers

#endif  // LFORTRAN_WASM_TO_WAT_H
