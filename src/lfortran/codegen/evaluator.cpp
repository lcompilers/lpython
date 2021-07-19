#include <iostream>
#include <fstream>

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
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetRegistry.h>

#include <lfortran/codegen/KaleidoscopeJIT.h>

#include <lfortran/codegen/evaluator.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/exception.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>


namespace LFortran {

template<typename T>
using Result = FortranEvaluator::Result<T>;

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
    return LFortran::LLVMEvaluator::module_to_string(*m_m);
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
        return "real";
    } else if (type->isIntegerTy()) {
        return "integer";
    } else if (type->isVoidTy()) {
        return "void";
    } else {
        throw LFortranException("Return type not supported");
    }
}

extern "C" {

void _lfortran_printf(const char* format, ...);

}

LLVMEvaluator::LLVMEvaluator()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    context = std::make_unique<llvm::LLVMContext>();

    target_triple = LLVMGetDefaultTargetTriple();

    std::string Error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, Error);
    if (!target) {
        throw LFortran::CodeGenError(Error);
    }
    std::string CPU = "generic";
    std::string features = "";
    llvm::TargetOptions opt;
    llvm::Optional<llvm::Reloc::Model> RM = llvm::Reloc::Model::PIC_;
    TM = target->createTargetMachine(target_triple, CPU, features, opt, RM);

    // For some reason the JIT requires a different TargetMachine
    llvm::TargetMachine *TM2 = llvm::EngineBuilder().selectTarget();
    jit = std::make_unique<llvm::orc::KaleidoscopeJIT>(TM2);

    llvm::sys::DynamicLibrary::AddSymbol("_lfortran_printf",
        (void*)
        reinterpret_cast<std::uintptr_t>(_lfortran_printf));
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
    mod->setDataLayout(jit->getTargetMachine().createDataLayout());
    jit->addModule(std::move(mod));
}

void LLVMEvaluator::add_module(std::unique_ptr<LLVMModule> m) {
    add_module(std::move(m->m_m));
}

intptr_t LLVMEvaluator::get_symbol_address(const std::string &name) {
    llvm::JITSymbol s = jit->findSymbol(name);
    if (!s) {
        throw std::runtime_error("findSymbol() failed to find the symbol '"
            + name + "'");
    }
    llvm::Expected<uint64_t> addr0 = s.getAddress();
    if (!addr0) {
        llvm::Error e = addr0.takeError();
        llvm::SmallVector<char, 128> buf;
        llvm::raw_svector_ostream dest(buf);
        llvm::logAllUnhandledErrors(std::move(e), dest, "");
        std::string msg = std::string(dest.str().data(), dest.str().size());
        if (msg[msg.size()-1] == '\n') msg = msg.substr(0, msg.size()-1);
        throw LFortranException("JITSymbol::getAddress() returned an error: " + msg);
    }
    return (intptr_t)cantFail(std::move(addr0));
}

int64_t LLVMEvaluator::intfn(const std::string &name) {
    intptr_t addr = get_symbol_address(name);
    int64_t (*f)() = (int64_t (*)())addr;
    return f();
}

bool LLVMEvaluator::boolfn(const std::string &name) {
    intptr_t addr = get_symbol_address(name);
    bool (*f)() = (bool (*)())addr;
    return f();
}

float LLVMEvaluator::floatfn(const std::string &name) {
    intptr_t addr = get_symbol_address(name);
    float (*f)() = (float (*)())addr;
    return f();
}

void LLVMEvaluator::voidfn(const std::string &name) {
    intptr_t addr = get_symbol_address(name);
    void (*f)() = (void (*)())addr;
    f();
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
    llvm::CodeGenFileType ft = llvm::CGFT_AssemblyFile;
    llvm::SmallVector<char, 128> buf;
    llvm::raw_svector_ostream dest(buf);
    if (jit->getTargetMachine().addPassesToEmitFile(pass, dest, nullptr, ft)) {
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
    llvm::CodeGenFileType ft = llvm::CGFT_ObjectFile;
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

llvm::LLVMContext &LLVMEvaluator::get_context()
{
    return *context;
}

/* ------------------------------------------------------------------------- */
// FortranEvaluator

FortranEvaluator::FortranEvaluator() : al{1024*1024},
    symbol_table{nullptr}, eval_count{0}
{
}

FortranEvaluator::~FortranEvaluator()
{
}

Result<FortranEvaluator::EvalResult> FortranEvaluator::evaluate(const std::string &code,
    bool verbose)
{
    try {
        EvalResult result;

        // Src -> AST
        LFortran::AST::TranslationUnit_t* ast;
        ast = LFortran::parse(al, code);

        if (verbose) {
            result.ast = LFortran::pickle(*ast, true);
        }

        // AST -> ASR
        LFortran::ASR::TranslationUnit_t* asr;
        // Remove the old execution function if it exists
        if (symbol_table) {
            if (symbol_table->scope.find(run_fn) != symbol_table->scope.end()) {
                symbol_table->scope.erase(run_fn);
            }
            symbol_table->mark_all_variables_external(al);
        }
        asr = LFortran::ast_to_asr(al, *ast, symbol_table);
        if (!symbol_table) symbol_table = asr->m_global_scope;

        if (verbose) {
            result.asr = LFortran::pickle(*asr, true);
        }

        eval_count++;
        run_fn = "__lfortran_evaluate_" + std::to_string(eval_count);

        // ASR -> LLVM
        std::unique_ptr<LFortran::LLVMModule> m;
        m = LFortran::asr_to_llvm(*asr, e.get_context(), al, run_fn);

        if (verbose) {
            result.llvm_ir = m->str();
        }

        std::string return_type = m->get_return_type(run_fn);

        // LLVM -> Machine code -> Execution
        e.add_module(std::move(m));
        if (return_type == "integer") {
            int r = e.intfn(run_fn);
            result.type = EvalResult::integer;
            result.i = r;
        } else if (return_type == "real") {
            float r = e.floatfn(run_fn);
            result.type = EvalResult::real;
            result.f = r;
        } else if (return_type == "void") {
            e.voidfn(run_fn);
            result.type = EvalResult::statement;
        } else if (return_type == "none") {
            result.type = EvalResult::none;
        } else {
            throw LFortran::LFortranException("Return type not supported");
        }
        return result;
    } catch (const TokenizerError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Tokenizer;
        error.loc = e.loc;
        error.msg = e.msg();
        error.token_str = e.token;
        return error;
    } catch (const ParserError &e) {
        int token;
        if (e.msg() == "syntax is ambiguous") {
            token = -2;
        } else {
            token = e.token;
        }
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Parser;
        error.loc = e.loc;
        error.token = token;
        error.msg = e.msg();
        return error;
    } catch (const SemanticError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Semantic;
        error.loc = e.loc;
        error.msg = e.msg();
        return error;
    } catch (const CodeGenError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::CodeGen;
        error.msg = e.msg();
        return error;
    }
}

Result<std::string> FortranEvaluator::get_ast(const std::string &code)
{
    Result<AST::TranslationUnit_t*> ast = get_ast2(code);
    if (ast.ok) {
        return LFortran::pickle(*ast.result, true);
    } else {
        return ast.error;
    }
}

Result<AST::TranslationUnit_t*> FortranEvaluator::get_ast2(const std::string &code)
{
    try {
        // Src -> AST
        LFortran::AST::TranslationUnit_t* ast;
        ast = LFortran::parse(al, code);
        return ast;
    } catch (const TokenizerError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Tokenizer;
        error.loc = e.loc;
        error.msg = e.msg();
        error.token_str = e.token;
        return error;
    } catch (const ParserError &e) {
        int token;
        if (e.msg() == "syntax is ambiguous") {
            token = -2;
        } else {
            token = e.token;
        }
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Parser;
        error.loc = e.loc;
        error.token = token;
        error.msg = e.msg();
        return error;
    }
}

Result<std::string> FortranEvaluator::get_asr(const std::string &code)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code);
    if (asr.ok) {
        return LFortran::pickle(*asr.result, true);
    } else {
        return asr.error;
    }
}

Result<ASR::TranslationUnit_t*> FortranEvaluator::get_asr2(const std::string &code)
{
    ASR::TranslationUnit_t* asr;
    try {
        // Src -> AST
        AST::TranslationUnit_t* ast;
        ast = parse(al, code);

        // AST -> ASR
        // Remove the old execution function if it exists
        if (symbol_table) {
            if (symbol_table->scope.find(run_fn) != symbol_table->scope.end()) {
                symbol_table->scope.erase(run_fn);
            }
            symbol_table->mark_all_variables_external(al);
        }
        asr = ast_to_asr(al, *ast, symbol_table);
        if (!symbol_table) symbol_table = asr->m_global_scope;
    } catch (const TokenizerError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Tokenizer;
        error.loc = e.loc;
        error.msg = e.msg();
        error.token_str = e.token;
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    } catch (const ParserError &e) {
        int token;
        if (e.msg() == "syntax is ambiguous") {
            token = -2;
        } else {
            token = e.token;
        }
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Parser;
        error.loc = e.loc;
        error.token = token;
        error.msg = e.msg();
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    } catch (const SemanticError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::Semantic;
        error.loc = e.loc;
        error.msg = e.msg();
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    } catch (const CodeGenError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::CodeGen;
        error.msg = e.msg();
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    }

    return asr;
}

Result<std::string> FortranEvaluator::get_llvm(const std::string &code)
{
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code);
    if (res.ok) {
        return res.result->str();
    } else {
        return res.error;
    }
}

Result<std::unique_ptr<LLVMModule>> FortranEvaluator::get_llvm2(const std::string &code)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code);
    if (!asr.ok) {
        return asr.error;
    }

    eval_count++;
    run_fn = "__lfortran_evaluate_" + std::to_string(eval_count);

    // ASR -> LLVM
    std::unique_ptr<LFortran::LLVMModule> m;
    try {
        m = LFortran::asr_to_llvm(*asr.result, e.get_context(), al, run_fn);
    } catch (const CodeGenError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::CodeGen;
        error.msg = e.msg();
        return error;
    }

    return m;
}

Result<std::string> FortranEvaluator::get_asm(const std::string &code)
{
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code);
    if (res.ok) {
        return e.get_asm(*res.result->m_m);
    } else {
        return res.error;
    }
}

Result<std::string> FortranEvaluator::get_cpp(const std::string &code)
{
    // Src -> AST -> ASR
    SymbolTable *old_symbol_table = symbol_table;
    symbol_table = nullptr;
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code);
    symbol_table = old_symbol_table;
    if (asr.ok) {
        // ASR -> C++
        return LFortran::asr_to_cpp(*asr.result);
    } else {
        return asr.error;
    }
}

Result<std::string> FortranEvaluator::get_fmt(const std::string &code)
{
    // Src -> AST
    Result<AST::TranslationUnit_t*> ast = get_ast2(code);
    if (ast.ok) {
        // AST -> Fortran
        return LFortran::ast_to_src(*ast.result, true);
    } else {
        return ast.error;
    }
}

std::string FortranEvaluator::format_error(const Error &e, const std::string &input,
    bool use_colors) const
{
    switch (e.type) {
        case (LFortran::FortranEvaluator::Error::Tokenizer) : {
            return format_syntax_error("input", input, e.loc, -1, &e.token_str, use_colors);
        }
        case (LFortran::FortranEvaluator::Error::Parser) : {
            return format_syntax_error("input", input, e.loc, e.token, nullptr, use_colors);
        }
        case (LFortran::FortranEvaluator::Error::Semantic) : {
            return format_semantic_error("input", input, e.loc, e.msg, use_colors);
        }
        case (LFortran::FortranEvaluator::Error::CodeGen) : {
            return "Code generation error: " + e.msg + "\n";
        }
        default : {
            throw LFortranException("Unknown error type");
        }
    }
}

std::string FortranEvaluator::error_stacktrace(const Error &e) const
{
    std::vector<StacktraceItem> d = e.stacktrace_addresses;
    get_local_addresses(d);
    get_local_info(d);
    return stacktrace2str(d, stacktrace_depth-1);
}

} // namespace LFortran
