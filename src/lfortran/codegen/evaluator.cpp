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

#include <lfortran/codegen/KaleidoscopeJIT.h>

#include <lfortran/codegen/evaluator.h>
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
    jit = std::make_unique<llvm::orc::KaleidoscopeJIT>();
}

LLVMEvaluator::~LLVMEvaluator()
{
    jit.reset();
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
    module->setDataLayout(jit->getTargetMachine().createDataLayout());
    return module;
}

void LLVMEvaluator::add_module(const std::string &source) {
    std::unique_ptr<llvm::Module> module = parse_module(source);
    // TODO: apply LLVM optimizations here
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "LLVM Module IR:" << std::endl;
    std::cout << module_to_string(*module);
    std::cout << "---------------------------------------------" << std::endl;
    add_module(std::move(module));
}

void LLVMEvaluator::add_module(std::unique_ptr<llvm::Module> mod) {
    // These are already set in parse_module(), but we set it here again for
    // cases when the Module was constructed directly, not via parse_module().
    mod->setTargetTriple(target_triple);
    mod->setDataLayout(jit->getTargetMachine().createDataLayout());
    jit->addModule(std::move(mod));
}

int64_t LLVMEvaluator::intfn(const std::string &name) {
    llvm::JITSymbol s = jit->findSymbol(name);
    if (!s) {
        throw std::runtime_error("Unable to get pointer to function");
    }
    int64_t (*f)() = (int64_t (*)())(intptr_t)cantFail(s.getAddress());
    return f();
}

void LLVMEvaluator::voidfn(const std::string &name) {
    llvm::JITSymbol s = jit->findSymbol(name);
    if (!s) {
        throw std::runtime_error("Unable to get pointer to function");
    }
    void (*f)() = (void (*)())(intptr_t)cantFail(s.getAddress());
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

    if (jit->getTargetMachine().addPassesToEmitFile(pass, dest, nullptr, ft)) {
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

llvm::LLVMContext &LLVMEvaluator::get_context()
{
    return *context;
}

} // namespace LFortran
