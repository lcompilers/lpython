#ifndef LFORTRAN_LLVM_UTILS_H
#define LFORTRAN_LLVM_UTILS_H

#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

namespace LFortran {

    class LLVMUtils {

        private:

            llvm::LLVMContext& context;
            llvm::IRBuilder<>* builder;
        
        public:

            LLVMUtils(llvm::LLVMContext& context,
                llvm::IRBuilder<>* _builder);

            llvm::Value* create_gep(llvm::Value* ds, int idx);

            llvm::Value* create_gep(llvm::Value* ds, llvm::Value* idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, int idx);

            llvm::Value* create_ptr_gep(llvm::Value* ptr, llvm::Value* idx);

    }; // LLVMUtils

} // LFortran

#endif // LFORTRAN_LLVM_UTILS_H
