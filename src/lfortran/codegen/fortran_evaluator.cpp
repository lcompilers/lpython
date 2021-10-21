#include <iostream>
#include <fstream>

#include <lfortran/codegen/fortran_evaluator.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/exception.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser.h>
#include <lfortran/parser/preprocessor.h>
#include <lfortran/pickle.h>

#ifdef HAVE_LFORTRAN_LLVM
#include <lfortran/codegen/evaluator.h>
#include <lfortran/codegen/asr_to_llvm.h>
#else
namespace LFortran {
    class LLVMEvaluator {};
}
#endif

namespace LFortran {

template<typename T>
using Result = FortranEvaluator::Result<T>;



/* ------------------------------------------------------------------------- */
// FortranEvaluator

FortranEvaluator::FortranEvaluator(CompilerOptions compiler_options)
    :
    al{1024*1024},
#ifdef HAVE_LFORTRAN_LLVM
    e{std::make_unique<LLVMEvaluator>()},
    eval_count{0},
#endif
    compiler_options{compiler_options},
    symbol_table{nullptr}
{
}

FortranEvaluator::~FortranEvaluator() = default;

Result<FortranEvaluator::EvalResult> FortranEvaluator::evaluate2(const std::string &code) {
    LocationManager lm;
    lm.in_filename = "input";
    return evaluate(code, false, lm);
}
Result<FortranEvaluator::EvalResult> FortranEvaluator::evaluate(
#ifdef HAVE_LFORTRAN_LLVM
            const std::string &code_orig, bool verbose, LocationManager &lm
#else
            const std::string &/*code_orig*/, bool /*verbose*/,
                LocationManager &/*lm*/
#endif
            )
{
#ifdef HAVE_LFORTRAN_LLVM
    EvalResult result;

    // Src -> AST
    Result<AST::TranslationUnit_t*> res = get_ast2(code_orig, lm);
    AST::TranslationUnit_t* ast;
    if (res.ok) {
        ast = res.result;
    } else {
        return res.error;
    }

    if (verbose) {
        result.ast = LFortran::pickle(*ast, true);
    }

    // AST -> ASR
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast);
    LFortran::ASR::TranslationUnit_t* asr;
    if (res2.ok) {
        asr = res2.result;
    } else {
        return res2.error;
    }

    if (verbose) {
        result.asr = LFortran::pickle(*asr, true);
    }

    // ASR -> LLVM
    Result<std::unique_ptr<LLVMModule>> res3 = get_llvm3(*asr);
    std::unique_ptr<LFortran::LLVMModule> m;
    if (res3.ok) {
        m = std::move(res3.result);
    } else {
        return res3.error;
    }

    if (verbose) {
        result.llvm_ir = m->str();
    }

    std::string return_type = m->get_return_type(run_fn);

    // LLVM -> Machine code -> Execution
    e->add_module(std::move(m));
    if (return_type == "integer4") {
        int32_t r = e->int32fn(run_fn);
        result.type = EvalResult::integer4;
        result.i32 = r;
    } else if (return_type == "integer8") {
        int64_t r = e->int64fn(run_fn);
        result.type = EvalResult::integer8;
        result.i64 = r;
    } else if (return_type == "real4") {
        float r = e->floatfn(run_fn);
        result.type = EvalResult::real4;
        result.f32 = r;
    } else if (return_type == "real8") {
        double r = e->doublefn(run_fn);
        result.type = EvalResult::real8;
        result.f64 = r;
    } else if (return_type == "complex4") {
        std::complex<float> r = e->complex4fn(run_fn);
        result.type = EvalResult::complex4;
        result.c32.re = r.real();
        result.c32.im = r.imag();
    } else if (return_type == "complex8") {
        std::complex<double> r = e->complex8fn(run_fn);
        result.type = EvalResult::complex8;
        result.c64.re = r.real();
        result.c64.im = r.imag();
    } else if (return_type == "void") {
        e->voidfn(run_fn);
        result.type = EvalResult::statement;
    } else if (return_type == "none") {
        result.type = EvalResult::none;
    } else {
        throw LFortranException("FortranEvaluator::evaluate(): Return type not supported");
    }
    return result;
#else
    throw LFortranException("LLVM is not enabled");
#endif
}

Result<std::string> FortranEvaluator::get_ast(const std::string &code,
    LocationManager &lm)
{
    Result<AST::TranslationUnit_t*> ast = get_ast2(code, lm);
    if (ast.ok) {
        return LFortran::pickle(*ast.result, compiler_options.use_colors,
            compiler_options.indent);
    } else {
        return ast.error;
    }
}

Result<AST::TranslationUnit_t*> FortranEvaluator::get_ast2(
            const std::string &code_orig, LocationManager &lm)
{
    try {
        // Src -> AST
        const std::string *code=&code_orig;
        std::string tmp;
        if (compiler_options.c_preprocessor) {
            // Preprocessor
            CPreprocessor cpp(compiler_options);
            tmp = cpp.run(code_orig, lm, cpp.macro_definitions);
            code = &tmp;
        }
        if (compiler_options.prescan || compiler_options.fixed_form) {
            tmp = fix_continuation(*code, lm, compiler_options.fixed_form);
            code = &tmp;
        }
        AST::TranslationUnit_t* ast;
        ast = parse(al, *code);
        return ast;
    } catch (const TokenizerError &e) {
        FortranEvaluator::Error error;
        error.d = e.d;
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    } catch (const ParserError &e) {
        FortranEvaluator::Error error;
        error.d = e.d;
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    }
}

Result<std::string> FortranEvaluator::get_asr(const std::string &code,
    LocationManager &lm)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code, lm);
    if (asr.ok) {
        return LFortran::pickle(*asr.result, true);
    } else {
        return asr.error;
    }
}

Result<ASR::TranslationUnit_t*> FortranEvaluator::get_asr2(
            const std::string &code_orig, LocationManager &lm)
{
    // Src -> AST
    Result<AST::TranslationUnit_t*> res = get_ast2(code_orig, lm);
    AST::TranslationUnit_t* ast;
    if (res.ok) {
        ast = res.result;
    } else {
        return res.error;
    }

    // AST -> ASR
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast);
    if (res2.ok) {
        return res2.result;
    } else {
        return res2.error;
    }
}

Result<ASR::TranslationUnit_t*> FortranEvaluator::get_asr3(
            AST::TranslationUnit_t &ast)
{
    ASR::TranslationUnit_t* asr;
    try {
        // AST -> ASR
        // Remove the old execution function if it exists
        if (symbol_table) {
            if (symbol_table->scope.find(run_fn) != symbol_table->scope.end()) {
                symbol_table->scope.erase(run_fn);
            }
            symbol_table->mark_all_variables_external(al);
        }
        asr = ast_to_asr(al, ast, symbol_table, compiler_options.symtab_only);
        if (!symbol_table) symbol_table = asr->m_global_scope;
    } catch (const SemanticError &e) {
        FortranEvaluator::Error error;
        error.d = e.d;
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
    }

    return asr;
}

Result<std::string> FortranEvaluator::get_llvm(
    const std::string &code, LocationManager &lm
    )
{
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code, lm);
    if (res.ok) {
#ifdef HAVE_LFORTRAN_LLVM
        return res.result->str();
#else
        throw LFortranException("LLVM is not enabled");
#endif
    } else {
        return res.error;
    }
}

Result<std::unique_ptr<LLVMModule>> FortranEvaluator::get_llvm2(
    const std::string &code, LocationManager &lm)
{
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code, lm);
    if (!asr.ok) {
        return asr.error;
    }
    Result<std::unique_ptr<LLVMModule>> res = get_llvm3(*asr.result);
    if (res.ok) {
#ifdef HAVE_LFORTRAN_LLVM
        std::unique_ptr<LLVMModule> m = std::move(res.result);
        return m;
#else
        throw LFortranException("LLVM is not enabled");
#endif
    } else {
        return res.error;
    }
}

Result<std::unique_ptr<LLVMModule>> FortranEvaluator::get_llvm3(
#ifdef HAVE_LFORTRAN_LLVM
    ASR::TranslationUnit_t &asr
#else
    ASR::TranslationUnit_t &/*asr*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    eval_count++;
    run_fn = "__lfortran_evaluate_" + std::to_string(eval_count);

    // ASR -> LLVM
    std::unique_ptr<LFortran::LLVMModule> m;
    Result<std::unique_ptr<LFortran::LLVMModule>> res
        = asr_to_llvm(asr, e->get_context(), al, compiler_options.platform,
            run_fn);
    if (res.ok) {
        m = std::move(res.result);
    } else {
        return res.error;
    }

    if (compiler_options.fast) {
        e->opt(*m->m_m);
    }

    return m;
#else
    throw LFortranException("LLVM is not enabled");
#endif
}

Result<std::string> FortranEvaluator::get_asm(
#ifdef HAVE_LFORTRAN_LLVM
    const std::string &code, LocationManager &lm
#else
    const std::string &/*code*/, LocationManager &/*lm*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code, lm);
    if (res.ok) {
        return e->get_asm(*res.result->m_m);
    } else {
        return res.error;
    }
#else
    throw LFortranException("LLVM is not enabled");
#endif
}

Result<std::string> FortranEvaluator::get_cpp(const std::string &code,
    LocationManager &lm)
{
    // Src -> AST -> ASR
    SymbolTable *old_symbol_table = symbol_table;
    symbol_table = nullptr;
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code, lm);
    symbol_table = old_symbol_table;
    if (asr.ok) {
        return get_cpp2(*asr.result);
    } else {
        return asr.error;
    }
}

Result<std::string> FortranEvaluator::get_cpp2(ASR::TranslationUnit_t &asr)
{
    // ASR -> C++
    return asr_to_cpp(asr);
}

Result<std::string> FortranEvaluator::get_fmt(const std::string &code,
    LocationManager &lm)
{
    // Src -> AST
    Result<AST::TranslationUnit_t*> ast = get_ast2(code, lm);
    if (ast.ok) {
        // AST -> Fortran
        return LFortran::ast_to_src(*ast.result, true);
    } else {
        return ast.error;
    }
}

std::string FortranEvaluator::format_error(const Error &e, const std::string &input,
        const LocationManager &lm) const
{
    std::string out;
    if (compiler_options.show_stacktrace) {
        out += error_stacktrace(e);
    }
    // Convert to line numbers and get source code strings
    diag::Diagnostic d = e.d;
    populate_spans(d, lm, input);
    // Render the message
    out += diag::render_diagnostic(d, compiler_options.use_colors);
    return out;
}

std::string FortranEvaluator::error_stacktrace(const Error &e) const
{
    std::vector<StacktraceItem> d = e.stacktrace_addresses;
    get_local_addresses(d);
    get_local_info(d);
    return stacktrace2str(d, stacktrace_depth-1);
}

} // namespace LFortran
