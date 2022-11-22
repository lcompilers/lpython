#ifndef LIBASR_WASM_TO_WAT_H
#define LIBASR_WASM_TO_WAT_H

#include <libasr/wasm_visitor.h>

namespace LCompilers {

Result<std::string> wasm_to_wat(Vec<uint8_t> &wasm_bytes, Allocator &al,
                                diag::Diagnostics &diagnostics);

}  // namespace LCompilers

#endif  // LIBASR_WASM_TO_WAT_H
