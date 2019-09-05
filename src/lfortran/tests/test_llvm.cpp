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

#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetRegistry.h"

#include <tests/doctest.h>

DEFINE_SIMPLE_CONVERSION_FUNCTIONS(llvm::TargetMachine, LLVMTargetMachineRef)


TEST_CASE("llvm 1") {
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
    bool v = llvm::verifyModule(*module);
    if (v) {
        std::cerr << "Error: module failed verification." << std::endl;
    };
    CHECK(v == false);
    std::cout << "The loaded module" << std::endl;
    llvm::outs() << *module;

    llvm::Function *f1 = module->getFunction("f1");

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(TargetTriple);
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    CHECK(Target != nullptr);
    auto CPU = "generic";
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TM =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    module->setDataLayout(TM->createDataLayout());


    /*
    std::cout << "ASM code" << std::endl;
    llvm::legacy::PassManager pass;
    llvm::TargetMachine::CodeGenFileType ft = llvm::TargetMachine::CGFT_AssemblyFile;
    std::error_code EC;
    llvm::raw_fd_ostream dest("output.txt", EC, llvm::sys::fs::OF_None);
    //CHECK(EC == 0);

    if (TM->addPassesToEmitFile(pass, dest, nullptr, ft)) {
        std::cout << "TargetMachine can't emit a file of this type" <<
            std::endl;
        CHECK(false);
    }
    pass.run(*module);
    */


    std::unique_ptr<llvm::ExecutionEngine> ee
        = std::unique_ptr<llvm::ExecutionEngine>(
                llvm::EngineBuilder(std::move(module)).create(TM));
    if (!ee) {
        std::cout << "Error: execution engine creation failed." << std::endl;
    }
    CHECK(ee != nullptr);
    ee->finalizeObject();
    std::vector<llvm::GenericValue> args;
    llvm::GenericValue gv = ee->runFunction(f1, args);

    std::string r = gv.IntVal.toString(10, true);

    std::cout << "Result: " << r << std::endl;

    CHECK(r == "4");

    ee.reset();
    llvm::llvm_shutdown();
}
