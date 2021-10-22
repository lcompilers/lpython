#ifndef LFORTRAN_FORTRAN_EVALUATOR_H
#define LFORTRAN_FORTRAN_EVALUATOR_H

#include <iostream>
#include <memory>

#include <lfortran/parser/alloc.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/asr_scopes.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/utils.h>
#include <lfortran/config.h>
#include <lfortran/diagnostics.h>

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

    // Evaluates `code`.
    // If `verbose=true`, it saves ast, asr and llvm_ir in Result.
    Result<EvalResult> evaluate(const std::string &code, bool verbose,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<EvalResult> evaluate2(const std::string &code);

    Result<std::string> get_ast(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<AST::TranslationUnit_t*> get_ast2(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<ASR::TranslationUnit_t*> get_asr3(AST::TranslationUnit_t &ast,
        diag::Diagnostics &diagnostics);
    Result<std::string> get_asr(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<ASR::TranslationUnit_t*> get_asr2(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<std::string> get_llvm(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<std::unique_ptr<LLVMModule>> get_llvm2(const std::string &code,
        LocationManager &lm, diag::Diagnostics &diagnostics);
    Result<std::unique_ptr<LLVMModule>> get_llvm3(ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics);
    Result<std::string> get_asm(const std::string &code, LocationManager &lm,
        diag::Diagnostics &diagnostics);
    Result<std::string> get_cpp(const std::string &code, LocationManager &lm,
        diag::Diagnostics &diagnostics);
    Result<std::string> get_cpp2(ASR::TranslationUnit_t &asr,
        diag::Diagnostics &diagnostics);
    Result<std::string> get_fmt(const std::string &code, LocationManager &lm,
        diag::Diagnostics &diagnostics);

    std::string format_error(const Error &e, const std::string &input,
            const LocationManager &lm) const;
    static std::string error_stacktrace(const std::vector<StacktraceItem> &stacktrace);

private:
    Allocator al;
#ifdef HAVE_LFORTRAN_LLVM
    std::unique_ptr<LLVMEvaluator> e;
    int eval_count;
#endif
    CompilerOptions compiler_options;
    SymbolTable *symbol_table;
    std::string run_fn;
};

} // namespace LFortran

#endif // LFORTRAN_FORTRAN_EVALUATOR_H
