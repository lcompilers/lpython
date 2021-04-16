#ifndef LFORTRAN_PASS_NESTED_VARS_H
#define LFORTRAN_PASS_NESTED_VARS_H

#include <lfortran/asr.h>
#include <llvm/IR/Type.h>

namespace LFortran {

     std::map<uint64_t, std::vector<llvm::Type*>> pass_find_nested_vars(
             ASR::TranslationUnit_t &unit, llvm::LLVMContext &context,
             std::vector<uint64_t> &needed_globals);

} // namespace LFortran

#endif // LFORTRAN_PASS_NESTED_VARS_H
