#ifndef LFORTRAN_ASR_TO_LLVM_H
#define LFORTRAN_ASR_TO_LLVM_H

#include <libasr/asr.h>
#include <libasr/codegen/evaluator.h>
#include <libasr/pass/pass_manager.h>

namespace LFortran {

    Result<std::unique_ptr<LLVMModule>> asr_to_llvm(ASR::TranslationUnit_t &asr,
            diag::Diagnostics &diagnostics,
            llvm::LLVMContext &context, Allocator &al,
            LCompilers::PassManager& pass_manager,
            CompilerOptions &compiler_options,
            const std::string &run_fn,
            const std::string &infile);

} // namespace LFortran

#endif // LFORTRAN_ASR_TO_LLVM_H
