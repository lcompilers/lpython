#ifndef LPYTHON_ASR_TO_PYTHON_H
#define LPYTHON_ASR_TO_PYTHON_H

#include <libasr/asr.h>
#include <libasr/asr_utils.h>

namespace LCompilers {

    // Convert ASR to Python source code
    std::string asr_to_lpython(Allocator &al, ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics, CompilerOptions &co);

} // namespace LCompilers

#endif // LPYTHON_ASR_TO_PYTHON_H
