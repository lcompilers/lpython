#ifndef LFORTRAN_ASR_TO_LLVM_H
#define LFORTRAN_ASR_TO_LLVM_H

#include <lfortran/asr.h>

namespace llvm {
    class Module;
}

namespace LFortran {

    std::unique_ptr<llvm::Module> asr_to_llvm(ASR::asr_t &asr);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_LLVM_H
