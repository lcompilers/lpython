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

Result<FortranEvaluator::EvalResult> FortranEvaluator::evaluate(
#ifdef HAVE_LFORTRAN_LLVM
            const std::string &code_orig, bool verbose
#else
            const std::string &/*code_orig*/, bool /*verbose*/
#endif
            )
{
#ifdef HAVE_LFORTRAN_LLVM
    try {
        EvalResult result;

        // Src -> AST
        LFortran::AST::TranslationUnit_t* ast;
        LFortran::LocationManager lm;
        std::string code = LFortran::fix_continuation(code_orig, lm, false);
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
        m = LFortran::asr_to_llvm(*asr, e->get_context(), al, compiler_options.platform, run_fn);

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
#else
    throw LFortranException("LLVM is not enabled");
#endif
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

Result<AST::TranslationUnit_t*> FortranEvaluator::get_ast2(
            const std::string &code_orig)
{
    try {
        // Src -> AST
        LFortran::AST::TranslationUnit_t* ast;
        LFortran::LocationManager lm;
        std::string code = LFortran::fix_continuation(code_orig, lm, false);
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

Result<ASR::TranslationUnit_t*> FortranEvaluator::get_asr2(
            const std::string &code_orig)
{
    ASR::TranslationUnit_t* asr;
    try {
        // Src -> AST
        AST::TranslationUnit_t* ast;
        LFortran::LocationManager lm;
        std::string code = LFortran::fix_continuation(code_orig, lm,
            compiler_options.fixed_form);
        ast = parse(al, code);

        // AST -> ASR
        // Remove the old execution function if it exists
        if (symbol_table) {
            if (symbol_table->scope.find(run_fn) != symbol_table->scope.end()) {
                symbol_table->scope.erase(run_fn);
            }
            symbol_table->mark_all_variables_external(al);
        }
        asr = ast_to_asr(al, *ast, symbol_table, compiler_options.symtab_only);
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

Result<std::string> FortranEvaluator::get_llvm(
#ifdef HAVE_LFORTRAN_LLVM
    const std::string &code
#else
    const std::string &/*code*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code);
    if (res.ok) {
        return res.result->str();
    } else {
        return res.error;
    }
#else
    throw LFortranException("LLVM is not enabled");
#endif
}

Result<std::unique_ptr<LLVMModule>> FortranEvaluator::get_llvm2(
#ifdef HAVE_LFORTRAN_LLVM
    const std::string &code
#else
    const std::string &/*code*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    Result<ASR::TranslationUnit_t*> asr = get_asr2(code);
    if (!asr.ok) {
        return asr.error;
    }

    eval_count++;
    run_fn = "__lfortran_evaluate_" + std::to_string(eval_count);

    // ASR -> LLVM
    std::unique_ptr<LFortran::LLVMModule> m;
    try {
        m = LFortran::asr_to_llvm(*asr.result, e->get_context(), al, compiler_options.platform,
            run_fn);
    } catch (const CodeGenError &e) {
        FortranEvaluator::Error error;
        error.type = FortranEvaluator::Error::CodeGen;
        error.msg = e.msg();
        error.stacktrace_addresses = e.stacktrace_addresses();
        return error;
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
    const std::string &code
#else
    const std::string &/*code*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    Result<std::unique_ptr<LLVMModule>> res = get_llvm2(code);
    if (res.ok) {
        return e->get_asm(*res.result->m_m);
    } else {
        return res.error;
    }
#else
    throw LFortranException("LLVM is not enabled");
#endif
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
        try {
            return LFortran::asr_to_cpp(*asr.result);
        } catch (const SemanticError &e) {
            // Note: the asr_to_cpp should only throw CodeGenError
            // but we currently do not have location information for
            // CodeGenError. We need to add it. Until then we can raise
            // SemanticError to get the location information.
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

std::string FortranEvaluator::format_error(const Error &e, const std::string &input) const
{
    std::string out;
    if (compiler_options.show_stacktrace) {
        out += error_stacktrace(e);
    }
    switch (e.type) {
        case (LFortran::FortranEvaluator::Error::Tokenizer) : {
            out += format_syntax_error("input", input, e.loc, -1, &e.token_str, compiler_options.use_colors);
            break;
        }
        case (LFortran::FortranEvaluator::Error::Parser) : {
            out += format_syntax_error("input", input, e.loc, e.token, nullptr, compiler_options.use_colors);
            break;
        }
        case (LFortran::FortranEvaluator::Error::Semantic) : {
            out += format_semantic_error("input", input, e.loc, e.msg, compiler_options.use_colors);
            break;
        }
        case (LFortran::FortranEvaluator::Error::CodeGen) : {
            out += "Code generation error: " + e.msg + "\n";
            break;
        }
        default : {
            throw LFortranException("Unknown error type");
        }
    }
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
