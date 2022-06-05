#ifndef LCOMPILERS_PASS_NESTED_VARS_H
#define LCOMPILERS_PASS_NESTED_VARS_H

#include <libasr/asr.h>
#include <llvm/IR/Type.h>

namespace LCompilers {

     std::map<uint64_t, std::vector<llvm::Type*>> pass_find_nested_vars(
             ASR::TranslationUnit_t &unit, llvm::LLVMContext &context,
             std::vector<uint64_t> &needed_globals,
             std::vector<uint64_t> &nested_call_out,
             std::map<uint64_t, std::vector<uint64_t>> &nesting_map);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_NESTED_VARS_H
