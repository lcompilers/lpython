#ifndef LCOMPILERS_ASR_TO_LLVM_H
#define LCOMPILERS_ASR_TO_LLVM_H

#include <libasr/asr.h>
#include <libasr/codegen/evaluator.h>

namespace LCompilers {

    Result<std::unique_ptr<LLVMModule>> asr_to_llvm(ASR::TranslationUnit_t &asr,
            diag::Diagnostics &diagnostics,
            llvm::LLVMContext &context, Allocator &al, Platform platform,
            bool fast, const std::string &rl_path, const std::string &run_fn);

} // namespace LCompilers

#endif // LCOMPILERS_ASR_TO_LLVM_H
