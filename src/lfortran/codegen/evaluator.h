#ifndef LFORTRAN_EVALUATOR_H
#define LFORTRAN_EVALUATOR_H

#include <iostream>
#include <memory>

#include <lfortran/parser/alloc.h>
#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/utils.h>

// Forward declare all needed LLVM classes without importing any LLVM header
// files. Those are only imported in evaluator.cpp and nowhere else, to speed
// up compilation.
namespace llvm {
    class ExecutionEngine;
    class LLVMContext;
    class Module;
    class Function;
    class TargetMachine;
    namespace orc {
        class KaleidoscopeJIT;
    }
}

namespace LFortran {

class LLVMModule
{
public:
    std::unique_ptr<llvm::Module> m_m;
    LLVMModule(std::unique_ptr<llvm::Module> m);
    ~LLVMModule();
    std::string str();
    // Return a function return type as a string (real / integer)
    std::string get_return_type(const std::string &fn_name);
};

class LLVMEvaluator
{
private:
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit;
    std::unique_ptr<llvm::LLVMContext> context;
    std::string target_triple;
    llvm::TargetMachine *TM;
public:
    LLVMEvaluator();
    ~LLVMEvaluator();
    std::unique_ptr<llvm::Module> parse_module(const std::string &source);
    void add_module(const std::string &source);
    void add_module(std::unique_ptr<llvm::Module> mod);
    void add_module(std::unique_ptr<LLVMModule> m);
    intptr_t get_symbol_address(const std::string &name);
    int64_t intfn(const std::string &name);
    bool boolfn(const std::string &name);
    float floatfn(const std::string &name);
    void voidfn(const std::string &name);
    std::string get_asm(llvm::Module &m);
    void save_asm_file(llvm::Module &m, const std::string &filename);
    void save_object_file(llvm::Module &m, const std::string &filename);
    void create_empty_object_file(const std::string &filename);
    static std::string module_to_string(llvm::Module &m);
    static void print_version_message();
    llvm::LLVMContext &get_context();
};

class FortranEvaluator
{
public:
    FortranEvaluator(Platform platform);
    ~FortranEvaluator();

    struct EvalResult {
        enum {
            integer, real, statement, none
        } type;
        union {
            int64_t i;
            float f;
        };
        std::string ast;
        std::string asr;
        std::string llvm_ir;
    };

    struct Error {
        enum {
            Tokenizer, Parser, Semantic, CodeGen
        } type;
        Location loc;
        int token;
        std::string msg;
        std::string token_str;
        std::vector<StacktraceItem> stacktrace_addresses;
    };

    template<typename T>
    struct Result {
        bool ok;
        union {
            T result;
            Error error;
        };
        // Default constructor
        Result() = delete;
        // Success result constructor
        Result(const T &result) : ok{true}, result{result} {}
        // Error result constructor
        Result(const Error &error) : ok{false}, error{error} {}
        // Destructor
        ~Result() {
            if (!ok) {
                error.~Error();
            }
        }
        // Copy constructor
        Result(const Result<T> &other) : ok{other.ok} {
            if (ok) {
                new(&result) T(other.result);
            } else {
                new(&error) Error(other.error);
            }
        }
        // Copy assignment
        Result<T>& operator=(const Result<T> &other) {
            ok = other.ok;
            if (ok) {
                new(&result) T(other.result);
            } else {
                new(&error) Error(other.error);
            }
            return *this;
        }
        // Move constructor
        Result(T &&result) : ok{true}, result{std::move(result)} {}
        // Move assignment
        Result<T>&& operator=(T &&other) = delete;
    };

    // Evaluates `code`.
    // If `verbose=true`, it saves ast, asr and llvm_ir in Result.
    Result<EvalResult> evaluate(const std::string &code, bool verbose=false);

    Result<std::string> get_ast(const std::string &code);
    Result<AST::TranslationUnit_t*> get_ast2(const std::string &code);
    Result<std::string> get_asr(const std::string &code);
    Result<ASR::TranslationUnit_t*> get_asr2(const std::string &code,
            bool fixed_form);
    Result<std::string> get_llvm(const std::string &code);
    Result<std::unique_ptr<LLVMModule>> get_llvm2(const std::string &code);
    Result<std::string> get_asm(const std::string &code);
    Result<std::string> get_cpp(const std::string &code);
    Result<std::string> get_fmt(const std::string &code);

    std::string format_error(const Error &e, const std::string &input, bool use_colors=true) const;
    std::string error_stacktrace(const Error &e) const;

private:
    Allocator al;
    LLVMEvaluator e;
    Platform platform;
    SymbolTable *symbol_table;
    int eval_count;
    std::string run_fn;
};

} // namespace LFortran

#endif // LFORTRAN_EVALUATOR_H
