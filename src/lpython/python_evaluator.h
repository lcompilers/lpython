#ifndef LFORTRAN_PYTHON_EVALUATOR_H
#define LFORTRAN_PYTHON_EVALUATOR_H

#include <iostream>
#include <memory>

#include <libasr/alloc.h>
#include <libasr/asr_scopes.h>
#include <libasr/asr.h>
#include <lpython/python_ast.h>
#include <lpython/utils.h>
#include <libasr/config.h>
#include <libasr/diagnostics.h>
#include <libasr/pass/pass_manager.h>

namespace LCompilers {

class LLVMModule;
class LLVMEvaluator;

/*
   PythonCompiler is the main class to access the Python compiler.

   This class is used for both interactive (.evaluate()) and non-interactive
   (.get_llvm2()) compilation. The methods return diagnostic messages (errors,
   warnings, style suggestions, ...) as an argument. One can use
   Diagnostic::render to render them.

   One can use get_asr2() to obtain the ASR and then hand it over to other
   backends by hand.
*/
class PythonCompiler
{
public:
    CompilerOptions compiler_options;

    PythonCompiler(CompilerOptions compiler_options);
    ~PythonCompiler();

    struct EvalResult {
        enum {
            integer1,
            integer2,
            unsignedInteger1,
            unsignedInteger2,
            integer4,
            integer8,
            unsignedInteger4,
            unsignedInteger8,
            real4,
            real8,
            complex4,
            complex8,
            boolean,
            string,
            structt,
            statement,
            none
        } type;
        union {
            int32_t i32;
            int64_t i64;
            uint32_t u32;
            uint64_t u64;
            bool b;
            float f32;
            double f64;
            char *str;
            struct {float re, im;} c32;
            struct {double re, im;} c64;
            struct {
                void *structure;
                ASR::ttype_t *ttype;
                size_t *offsets;
                size_t element_size;
            } structure;
        };
        std::string ast;
        std::string asr;
        std::string llvm_ir;
    };

    Result<PythonCompiler::EvalResult> evaluate(
            const std::string &code_orig, bool verbose, LocationManager &lm,
            LCompilers::PassManager& pass_manager, diag::Diagnostics &diagnostics);

    Result<PythonCompiler::EvalResult> evaluate2(const std::string &code);

    Result<std::string> get_ast(const std::string &code,
            LocationManager &lm, diag::Diagnostics &diagnostics);

    Result<LCompilers::LPython::AST::ast_t*> get_ast2(
            const std::string &code_orig, diag::Diagnostics &diagnostics);

    Result<std::string> get_asr(const std::string &code,
            LocationManager &lm, diag::Diagnostics &diagnostics);

    Result<ASR::TranslationUnit_t*> get_asr2(
            const std::string &code_orig, LocationManager &lm,
            diag::Diagnostics &diagnostics);

    Result<ASR::TranslationUnit_t*> get_asr3(
        LCompilers::LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics,
        LocationManager &lm, bool is_interactive=false);

    Result<std::string> get_llvm(
            const std::string &code, LocationManager &lm, LCompilers::PassManager& pass_manager,
            diag::Diagnostics &diagnostics);

    Result<std::unique_ptr<LLVMModule>> get_llvm2(
            const std::string &code, LocationManager &lm, LCompilers::PassManager& pass_manager,
            diag::Diagnostics &diagnostics);

    Result<std::unique_ptr<LLVMModule>> get_llvm3(ASR::TranslationUnit_t &asr,
        LCompilers::PassManager& lpm, diag::Diagnostics &diagnostics,
        const std::string &infile);

    Result<std::string> get_asm(const std::string &code,
            LocationManager &lm,
            LCompilers::PassManager& pass_manager,
            diag::Diagnostics &diagnostics);

    std::string string_aggregate_type(const struct EvalResult &r);

private:
    Allocator al;
#ifdef HAVE_LFORTRAN_LLVM
    std::unique_ptr<LLVMEvaluator> e;
#endif
    int eval_count;
    SymbolTable *symbol_table;
    std::string run_fn;
};

} // namespace LCompilers

#endif // LFORTRAN_PYTHON_EVALUATOR_H
