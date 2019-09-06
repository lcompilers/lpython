#include <iostream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetRegistry.h>

#include <lfortran/evaluator.h>
#include <lfortran/exception.h>


namespace LFortran {

// Extracts the integer from APInt.
// APInt does not seem to have this functionality, so we implement it here.
uint64_t APInt_getint(const llvm::APInt &i) {
    // The APInt::isSingleWord() is private, but we can emulate it:
    bool isSingleWord = !i.needsCleanup();
    if (isSingleWord) {
        return *i.getRawData();
    } else {
        throw std::runtime_error("APInt too large to fit uint64_t");
    }
}

LLVMEvaluator::LLVMEvaluator()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    context = std::make_unique<llvm::LLVMContext>();

    target_triple = llvm::sys::getDefaultTargetTriple();
    std::string Error;
    const llvm::Target *Target = llvm::TargetRegistry::lookupTarget(target_triple, Error);
    if (Target == nullptr) {
        throw std::runtime_error("lookupTarget failed");
    }
    std::string CPU = "generic";
    std::string Features = "";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    TM = Target->createTargetMachine(target_triple, CPU, Features, opt, RM);

    std::unique_ptr<llvm::Module> module0 = parse_module("");
    ee = std::unique_ptr<llvm::ExecutionEngine>(
                llvm::EngineBuilder(std::move(module0)).create(TM));
    if (!ee) {
        std::runtime_error("Error: execution engine creation failed.");
    }
    ee->finalizeObject();
}

LLVMEvaluator::~LLVMEvaluator()
{
    // Ensure ExecutionEngine is deleted before LLVMContext
    ee.reset();
    context.reset();
}

std::unique_ptr<llvm::Module> LLVMEvaluator::parse_module(const std::string &source)
{
    llvm::SMDiagnostic err;
    std::unique_ptr<llvm::Module> module
        = llvm::parseAssemblyString(source, err, *context);
    if (!module) {
        throw CodeGenError("Invalid LLVM IR");
    }
    bool v = llvm::verifyModule(*module);
    if (v) {
        std::cerr << "Error: module failed verification." << std::endl;
        throw std::runtime_error("add_module");
    };
    module->setTargetTriple(target_triple);
    module->setDataLayout(TM->createDataLayout());
    return module;
}

void LLVMEvaluator::add_module(const std::string &source) {
    std::unique_ptr<llvm::Module> module = parse_module(source);
    // TODO: apply LLVM optimizations here
    add_module(std::move(module));
}

void LLVMEvaluator::add_module(std::unique_ptr<llvm::Module> mod) {
    ee->addModule(std::move(mod));
    ee->finalizeObject();
}

int64_t LLVMEvaluator::intfn(const std::string &name) {
    uint64_t ptr = ee->getFunctionAddress(name);
    if (ptr == 0) {
        throw std::runtime_error("Unable to get pointer to function");
    }
    int64_t (*f)() = (int64_t (*)())ptr;
    return f();
}

void LLVMEvaluator::voidfn(const std::string &name) {
    uint64_t ptr = ee->getFunctionAddress(name);
    if (ptr == 0) {
        throw std::runtime_error("Unable to get pointer to function");
    }
    void (*f)() = (void (*)())ptr;
    f();
}

void LLVMEvaluator::save_object_file(llvm::Module &m, const std::string &filename) {
    llvm::legacy::PassManager pass;
    //llvm::TargetMachine::CodeGenFileType ft = llvm::TargetMachine::CGFT_AssemblyFile;
    llvm::TargetMachine::CodeGenFileType ft = llvm::TargetMachine::CGFT_ObjectFile;
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        throw std::runtime_error("raw_fd_ostream failed");
    }

    if (TM->addPassesToEmitFile(pass, dest, nullptr, ft)) {
        throw std::runtime_error("TargetMachine can't emit a file of this type");
    }
    pass.run(m);
    dest.flush();
}

std::string LLVMEvaluator::module_to_string(llvm::Module &m) {
    std::string buf;
    llvm::raw_string_ostream os(buf);
    m.print(os, nullptr);
    os.flush();
    return buf;
}

void LLVMEvaluator::print_version_message()
{
    llvm::cl::PrintVersionMessage();
}

} // namespace LFortran
