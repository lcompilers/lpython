#ifndef LFORTRAN_ASR_TO_LLVM_H
#define LFORTRAN_ASR_TO_LLVM_H

#include <lfortran/asr.h>
#include <lfortran/codegen/evaluator.h>

namespace LFortran {

    std::unique_ptr<LLVMModule> asr_to_llvm(ASR::asr_t &asr,
            llvm::LLVMContext &context);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_LLVM_H
