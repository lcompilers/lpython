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
#include "llvm/AsmParser/Parser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/StringRef.h"


int main() {
    std::cout << "LLVM Version:" << std::endl;
    llvm::cl::PrintVersionMessage();

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::shared_ptr<llvm::LLVMContext> context
        = std::make_shared<llvm::LLVMContext>();

    llvm::SMDiagnostic err;
    std::string asm_string = R"""(;
define i64 @f1()
{
    ret i64 4
}
    )""";
    std::unique_ptr<llvm::Module> module
        = llvm::parseAssemblyString(asm_string, err, *context);
    if (llvm::verifyModule(*module)) {
        std::cerr << "Error: module failed verification." << std::endl;
    };
    std::cout << "The loaded module" << std::endl;
    llvm::outs() << *module;

    llvm::Function *f1 = module->getFunction("f1");

    std::unique_ptr<llvm::ExecutionEngine> ee
        = std::unique_ptr<llvm::ExecutionEngine>(
                llvm::EngineBuilder(std::move(module)).create());
    if (!ee) {
        std::cout << "Error: execution engine creation failed." << std::endl;
    }
    ee->finalizeObject();
    std::vector<llvm::GenericValue> args;
    llvm::GenericValue gv = ee->runFunction(f1, args);

    llvm::outs() << "Result: " << gv.IntVal << "\n";

    ee.reset();
    llvm::llvm_shutdown();
    return 0;
}
