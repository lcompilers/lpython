#include <iostream>
#include <fstream>

#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
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
#include <llvm/Transforms/Scalar/InstSimplifyPass.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Instrumentation/AddressSanitizer.h>
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Target/TargetOptions.h>
#if LLVM_VERSION_MAJOR >= 14
#    include <llvm/MC/TargetRegistry.h>
#else
#    include <llvm/Support/TargetRegistry.h>
#endif
#if LLVM_VERSION_MAJOR >= 17
    // TODO: removed from LLVM 17
    #include <llvm/Passes/PassBuilder.h>
#else
#    include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

#if LLVM_VERSION_MAJOR < 18
#    include <llvm/Transforms/Vectorize.h>
#    include <llvm/Support/Host.h>
#endif

#include <libasr/codegen/KaleidoscopeJIT.h>
#include <libasr/codegen/evaluator.h>
#include <libasr/codegen/asr_to_llvm.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr.h>
#include <libasr/string_utils.h>

#ifdef HAVE_LFORTRAN_MLIR
#include <mlir/IR/BuiltinOps.h>
#include <mlir/Target/LLVMIR/Export.h>
#endif

namespace LCompilers {

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

LLVMModule::LLVMModule(std::unique_ptr<llvm::Module> m)
{
    m_m = std::move(m);
}

LLVMModule::~LLVMModule() = default;

std::string LLVMModule::str()
{
    return LLVMEvaluator::module_to_string(*m_m);
}

llvm::Function *LLVMModule::get_function(const std::string &fn_name) {
    llvm::Module *m = m_m.get();
    return m->getFunction(fn_name);
}

llvm::GlobalVariable *LLVMModule::get_global(const std::string &global_name) {
    llvm::Module *m = m_m.get();
    return m->getNamedGlobal(global_name);
}

std::string LLVMModule::get_return_type(const std::string &fn_name)
{
    llvm::Module *m = m_m.get();
    llvm::Function *fn = m->getFunction(fn_name);
    if (!fn) {
        return "none";
    }
    llvm::Type *type = fn->getReturnType();
    if (type->isFloatTy()) {
        return "real4";
    } else if (type->isDoubleTy()) {
        return "real8";
    } else if (type->isIntegerTy(1)) {
        return "logical";
    } else if (type->isIntegerTy(32)) {
        return "integer4";
    } else if (type->isIntegerTy(64)) {
        return "integer8";
    } else if (type->isStructTy()) {
        llvm::StructType *st = llvm::cast<llvm::StructType>(type);
        if (st->hasName()) {
            if (startswith(std::string(st->getName()), "complex_4")) {
                return "complex4";
            } else if (startswith(std::string(st->getName()), "complex_8")) {
                return "complex8";
            } else {
                throw LCompilersException("LLVMModule::get_return_type(): StructType return type `" + std::string(st->getName()) + "` not supported");
            }
        } else {
            throw LCompilersException("LLVMModule::get_return_type(): Noname struct return type not supported");
        }
    } else if (type->isVectorTy()) {
        // Used for passing complex_4 on some platforms
        return "complex4";
    } else if (type->isVoidTy()) {
        return "void";
    } else {
        throw LCompilersException("LLVMModule::get_return_type(): Return type not supported");
    }
}

#ifdef HAVE_LFORTRAN_MLIR
MLIRModule::MLIRModule(std::unique_ptr<mlir::ModuleOp> m,
        std::unique_ptr<mlir::MLIRContext> ctx) {
    mlir_m = std::move(m);
    mlir_ctx = std::move(ctx);
    llvm_ctx = std::make_unique<llvm::LLVMContext>();
}

MLIRModule::~MLIRModule() {
    llvm_m.reset();
    llvm_ctx.reset();
};

std::string MLIRModule::mlir_str() {
    std::string mlir_str;
    llvm::raw_string_ostream raw_os(mlir_str);
    mlir_m->print(raw_os);
    return mlir_str;
}

std::string MLIRModule::llvm_str() {
    std::string mlir_str;
    llvm::raw_string_ostream raw_os(mlir_str);
    llvm_m->print(raw_os, nullptr);
    return mlir_str;
}

void MLIRModule::mlir_to_llvm(llvm::LLVMContext &ctx) {
    std::unique_ptr<llvm::Module> llvmModule = mlir::translateModuleToLLVMIR(
        *mlir_m, ctx);
    if (llvmModule) {
        llvm_m = std::move(llvmModule);
    } else {
        throw LCompilersException("Failed to generate LLVM IR");
    }
}
#endif

extern "C" {

float _lfortran_stan(float x);

}

LLVMEvaluator::LLVMEvaluator(const std::string &t)
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

#ifdef HAVE_TARGET_AARCH64
    LLVMInitializeAArch64Target();
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64AsmPrinter();
    LLVMInitializeAArch64AsmParser();
#endif
#ifdef HAVE_TARGET_X86
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
#endif
#ifdef HAVE_TARGET_WASM
    LLVMInitializeWebAssemblyTarget();
    LLVMInitializeWebAssemblyTargetInfo();
    LLVMInitializeWebAssemblyTargetMC();
    LLVMInitializeWebAssemblyAsmPrinter();
    LLVMInitializeWebAssemblyAsmParser();
#endif

    context = std::make_unique<llvm::LLVMContext>();

    if (t != "")
        target_triple = t;
    else
        target_triple = LLVMGetDefaultTargetTriple();

    std::string Error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, Error);
    if (!target) {
        throw LCompilersException(Error);
    }
    std::string CPU = "generic";
    std::string features = "";
    llvm::TargetOptions opt;
    RM_OPTIONAL_TYPE<llvm::Reloc::Model> RM = llvm::Reloc::Model::PIC_;
    TM = target->createTargetMachine(target_triple, CPU, features, opt, RM);

    // For some reason the JIT requires a different TargetMachine
    jit = cantFail(llvm::orc::KaleidoscopeJIT::Create());

    _lfortran_stan(0.5);
}

LLVMEvaluator::~LLVMEvaluator()
{
    jit.reset();
    context.reset();
}

std::unique_ptr<llvm::Module> LLVMEvaluator::parse_module(const std::string &source, const std::string &filename="")
{
    llvm::SMDiagnostic err;
    std::unique_ptr<llvm::Module> module;
    if (!filename.empty()) {
        module = llvm::parseAssemblyFile(filename, err, *context);
    } else {
        module = llvm::parseAssemblyString(source, err, *context);
    }
    if (!module) {
        err.print("", llvm::errs());
        throw LCompilersException("parse_module(): Invalid LLVM IR");
    }
    bool v = llvm::verifyModule(*module);
    if (v) {
        throw LCompilersException("parse_module(): module failed verification.");
    };
    module->setTargetTriple(target_triple);
    module->setDataLayout(jit->getDataLayout());
    return module;
}

std::unique_ptr<LLVMModule> LLVMEvaluator::parse_module2(const std::string &source, const std::string &filename="") {
    return std::make_unique<LLVMModule>(parse_module(source, filename));
}

void LLVMEvaluator::add_module(const std::string &source) {
    std::unique_ptr<llvm::Module> module = parse_module(source);
    // TODO: apply LLVM optimizations here
    // Uncomment the below code to print the module to stdout:
    /*
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "LLVM Module IR:" << std::endl;
    std::cout << module_to_string(*module);
    std::cout << "---------------------------------------------" << std::endl;
    */
    add_module(std::move(module));
}

void LLVMEvaluator::add_module(std::unique_ptr<llvm::Module> mod) {
    // These are already set in parse_module(), but we set it here again for
    // cases when the Module was constructed directly, not via parse_module().
    mod->setTargetTriple(target_triple);
    mod->setDataLayout(jit->getDataLayout());
    llvm::Error err = jit->addModule(std::move(mod), context);
    if (err) {
        llvm::SmallVector<char, 128> buf;
        llvm::raw_svector_ostream dest(buf);
        llvm::logAllUnhandledErrors(std::move(err), dest, "");
        std::string msg = std::string(dest.str().data(), dest.str().size());
        if (msg[msg.size()-1] == '\n') msg = msg.substr(0, msg.size()-1);
        throw LCompilersException("addModule() returned an error: " + msg);
    }

}

void LLVMEvaluator::add_module(std::unique_ptr<LLVMModule> m) {
    add_module(std::move(m->m_m));
}

intptr_t LLVMEvaluator::get_symbol_address(const std::string &name) {
#if LLVM_VERSION_MAJOR < 17
    llvm::Expected<llvm::JITEvaluatedSymbol>
#else
    llvm::Expected<llvm::orc::ExecutorSymbolDef>
#endif
        s = jit->lookup(name);
    if (!s) {
        llvm::Error e = s.takeError();
        llvm::SmallVector<char, 128> buf;
        llvm::raw_svector_ostream dest(buf);
        llvm::logAllUnhandledErrors(std::move(e), dest, "");
        std::string msg = std::string(dest.str().data(), dest.str().size());
        if (msg[msg.size()-1] == '\n') msg = msg.substr(0, msg.size()-1);
        throw LCompilersException("lookup() failed to find the symbol '"
            + name + "', error: " + msg);
    }
#if LLVM_VERSION_MAJOR < 17
    llvm::Expected<uint64_t> addr0 = s->getAddress();
#else
    llvm::Expected<uint64_t> addr0 = s->getAddress().getValue();
#endif
    if (!addr0) {
        llvm::Error e = addr0.takeError();
        llvm::SmallVector<char, 128> buf;
        llvm::raw_svector_ostream dest(buf);
        llvm::logAllUnhandledErrors(std::move(e), dest, "");
        std::string msg = std::string(dest.str().data(), dest.str().size());
        if (msg[msg.size()-1] == '\n') msg = msg.substr(0, msg.size()-1);
        throw LCompilersException("JITSymbol::getAddress() returned an error: " + msg);
    }
    return (intptr_t)cantFail(std::move(addr0));
}

void write_file(const std::string &filename, const std::string &contents)
{
    std::ofstream out;
    out.open(filename);
    out << contents << std::endl;
}

std::string LLVMEvaluator::get_asm(llvm::Module &m)
{
    llvm::legacy::PassManager pass;
#if LLVM_VERSION_MAJOR < 18
    llvm::CodeGenFileType ft = llvm::CGFT_AssemblyFile;
#else
    llvm::CodeGenFileType ft = llvm::CodeGenFileType::AssemblyFile;
#endif
    llvm::SmallVector<char, 128> buf;
    llvm::raw_svector_ostream dest(buf);
    if (TM->addPassesToEmitFile(pass, dest, nullptr, ft)) {
        throw std::runtime_error("TargetMachine can't emit a file of this type");
    }
    pass.run(m);
    return std::string(dest.str().data(), dest.str().size());
}

void LLVMEvaluator::save_asm_file(llvm::Module &m, const std::string &filename)
{
    write_file(filename, get_asm(m));
}

void LLVMEvaluator::save_object_file(llvm::Module &m, const std::string &filename) {
    m.setTargetTriple(target_triple);
    m.setDataLayout(TM->createDataLayout());

    llvm::legacy::PassManager pass;
#if LLVM_VERSION_MAJOR < 18
    llvm::CodeGenFileType ft = llvm::CGFT_ObjectFile;
#else
    llvm::CodeGenFileType ft = llvm::CodeGenFileType::ObjectFile;
#endif
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

void LLVMEvaluator::create_empty_object_file(const std::string &filename) {
    std::string source;
    std::unique_ptr<llvm::Module> module = parse_module(source);
    save_object_file(*module, filename);
}

void LLVMEvaluator::opt(llvm::Module &m) {
    m.setTargetTriple(target_triple);
    m.setDataLayout(TM->createDataLayout());

#if LLVM_VERSION_MAJOR >= 17
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;
    llvm::PassBuilder PB = llvm::PassBuilder(TM);
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    MPM.run(m, MAM);

#else
    llvm::legacy::PassManager mpm;
    mpm.add(new llvm::TargetLibraryInfoWrapperPass(TM->getTargetTriple()));
    mpm.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
    llvm::legacy::FunctionPassManager fpm(&m);
    fpm.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
    int optLevel = 3;
    int sizeLevel = 0;
    llvm::PassManagerBuilder builder;
    builder.OptLevel = optLevel;
    builder.SizeLevel = sizeLevel;
    builder.Inliner = llvm::createFunctionInliningPass(optLevel, sizeLevel,
        false);
    builder.DisableUnrollLoops = false;
    builder.LoopVectorize = true;
    builder.SLPVectorize = true;
    builder.populateFunctionPassManager(fpm);
    builder.populateModulePassManager(mpm);
    fpm.doInitialization();
    for (llvm::Function &func : m) {
        fpm.run(func);
    }
    fpm.doFinalization();
    mpm.add(llvm::createVerifierPass());
    mpm.run(m);
#endif
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

std::string LLVMEvaluator::llvm_version()
{
    return LLVM_VERSION_STRING;
}

llvm::LLVMContext &LLVMEvaluator::get_context()
{
    return *context;
}

const llvm::DataLayout &LLVMEvaluator::get_jit_data_layout() {
    return jit->getDataLayout();
}

void LLVMEvaluator::print_targets()
{
    llvm::InitializeNativeTarget();
#ifdef HAVE_TARGET_AARCH64
    LLVMInitializeAArch64TargetInfo();
#endif
#ifdef HAVE_TARGET_X86
    LLVMInitializeX86TargetInfo();
#endif
#ifdef HAVE_TARGET_WASM
    LLVMInitializeWebAssemblyTargetInfo();
#endif
    llvm::raw_ostream &os = llvm::outs();
    llvm::TargetRegistry::printRegisteredTargetsForVersion(os);
}

std::string LLVMEvaluator::get_default_target_triple()
{
    return LLVMGetDefaultTargetTriple();
}

} // namespace LCompilers
