#ifndef LFORTRAN_ASR_TO_LLVM_H
#define LFORTRAN_ASR_TO_LLVM_H

#include <lfortran/asr.h>
#include <lfortran/codegen/evaluator.h>

namespace LFortran {

    std::unique_ptr<LLVMModule> asr_to_llvm(ASR::TranslationUnit_t &asr,
            llvm::LLVMContext &context, Allocator &al, Platform platform,
            std::string run_fn="f");

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_LLVM_H
