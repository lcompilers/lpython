#ifndef LFORTRAN_FORTRAN_EVALUATOR_H
#define LFORTRAN_FORTRAN_EVALUATOR_H

#include <iostream>
#include <memory>

#include <lfortran/parser/alloc.h>
#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/utils.h>
#include <lfortran/config.h>

namespace LFortran {

class LLVMModule;
class LLVMEvaluator;

class FortranEvaluator
{
public:
    FortranEvaluator(CompilerOptions compiler_options);
    ~FortranEvaluator();

    struct EvalResult {
        enum {
            integer4, integer8, real4, real8, complex4, complex8, statement, none
        } type;
        union {
            int32_t i32;
            int64_t i64;
            float f32;
            double f64;
            struct {float re, im;} c32;
            struct {double re, im;} c64;
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
    Result<std::string> get_llvm(const std::string &code,
        bool fast);
    Result<std::unique_ptr<LLVMModule>> get_llvm2(const std::string &code,
        bool fast);
    Result<std::string> get_asm(const std::string &code,
        bool fast);
    Result<std::string> get_cpp(const std::string &code);
    Result<std::string> get_fmt(const std::string &code);

    std::string format_error(const Error &e, const std::string &input, bool use_colors=true) const;
    std::string error_stacktrace(const Error &e) const;

private:
    Allocator al;
#ifdef HAVE_LFORTRAN_LLVM
    std::unique_ptr<LLVMEvaluator> e;
    Platform platform;
    int eval_count;
#endif
    SymbolTable *symbol_table;
    std::string run_fn;
};

} // namespace LFortran

#endif // LFORTRAN_FORTRAN_EVALUATOR_H
