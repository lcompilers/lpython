#include <iostream>

#include <llvm/IR/LLVMContext.h>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar.h"
#include <llvm/Transforms/Scalar/GVN.h>
#include "llvm/Transforms/Vectorize.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"


int main() {
    std::cout << "LLVM Version:" << std::endl;
    llvm::cl::PrintVersionMessage();

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto context = std::make_shared<llvm::LLVMContext>();
    std::unique_ptr<llvm::Module> module
        = llvm::make_unique<llvm::Module>("SymEngine", *context.get());
    module->setDataLayout("");
    auto mod = module.get();

    return 0;
}
