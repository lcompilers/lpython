#include <lfortran/codegen/llvm_utils.h>

namespace LFortran {

    llvm::Value* LLVMUtils::create_gep(llvm::Value* ds, int idx,
        std::unique_ptr<llvm::IRBuilder<>> builder) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
        return builder->CreateGEP(ds, idx_vec);
    }

    llvm::Value* LLVMUtils::create_gep(llvm::Value* ds, llvm::Value* idx,
        std::unique_ptr<llvm::IRBuilder<>> builder) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
        idx};
        return builder->CreateGEP(ds, idx_vec);
    }

    llvm::Value* LLVMUtils::create_ptr_gep(llvm::Value* ptr, int idx,
        std::unique_ptr<llvm::IRBuilder<>> builder) {
        std::vector<llvm::Value*> idx_vec = {
        llvm::ConstantInt::get(context, llvm::APInt(32, idx))};
        return builder->CreateInBoundsGEP(ptr, idx_vec);
    }

    llvm::Value* LLVMUtils::create_ptr_gep(llvm::Value* ptr, llvm::Value* idx,
        std::unique_ptr<llvm::IRBuilder<>> builder) {
        std::vector<llvm::Value*> idx_vec = {idx};
        return builder->CreateInBoundsGEP(ptr, idx_vec);
    }

} // LFortran

#endif // LFORTRAN_LLVM_UTILS_H
