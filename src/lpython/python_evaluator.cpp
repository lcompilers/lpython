#include "libasr/asr_scopes.h"
#include "lpython/python_ast.h"
#include <iostream>

#include <lpython/python_evaluator.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr.h>
#include <lpython/pickle.h>
#include <lpython/parser/parser.h>
#include <string>

#ifdef HAVE_LFORTRAN_LLVM
#include <libasr/codegen/evaluator.h>
#include <libasr/codegen/asr_to_llvm.h>
#else
namespace LCompilers {
    class LLVMEvaluator {};
}
#endif

namespace LCompilers {


/* ------------------------------------------------------------------------- */
// PythonEvaluator

PythonCompiler::PythonCompiler(CompilerOptions compiler_options)
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

PythonCompiler::~PythonCompiler() = default;


Result<PythonCompiler::EvalResult> PythonCompiler::evaluate(
#ifdef HAVE_LFORTRAN_LLVM
            const std::string &code_orig, bool verbose, LocationManager &lm,
            LCompilers::PassManager& pass_manager, diag::Diagnostics &diagnostics
#else
            const std::string &/*code_orig*/, bool /*verbose*/,
                LocationManager &/*lm*/, LCompilers::PassManager& /*pass_manager*/,
                diag::Diagnostics &/*diagnostics*/
#endif
            )
{
#ifdef HAVE_LFORTRAN_LLVM
    EvalResult result;
    result.type = EvalResult::none;

    // Src -> AST
    Result<LCompilers::LPython::AST::ast_t*> res = get_ast2(code_orig, diagnostics);
    LCompilers::LPython::AST::ast_t* ast;
    if (res.ok) {
        ast = res.result;
    } else {
        return res.error;
    }

    if (verbose) {
        result.ast = LCompilers::LPython::pickle_python(*ast, true, true);
    }

    // AST -> ASR
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast, diagnostics, lm);
    ASR::TranslationUnit_t* asr;
    if (res2.ok) {
        asr = res2.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res2.error;
    }

    if (verbose) {
        result.asr = pickle(*asr, true, true, true);
    }

    // ASR -> LLVM
    Result<std::unique_ptr<LLVMModule>> res3 = get_llvm3(*asr,
        pass_manager, diagnostics, lm.files.back().in_filename);
    std::unique_ptr<LCompilers::LLVMModule> m;
    if (res3.ok) {
        m = std::move(res3.result);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res3.error;
    }

    if (verbose) {
        result.llvm_ir = m->str();
    }

    bool call_init = false;
    bool call_stmts = false;
    std::string init_fn = "__module___main_____main____lpython_interactive_init_" + std::to_string(eval_count) + "__";
    std::string stmts_fn = "__module___main_____main____lpython_interactive_stmts_" + std::to_string(eval_count) + "__";
    if (m->get_return_type(init_fn) != "none") {
        call_init = true;
    }
    if (m->get_return_type(stmts_fn) != "none") {
        call_stmts = true;
    }

    e->add_module(std::move(m));
    if (call_init) {
        e->voidfn(init_fn);
    }
    if (call_stmts) {
        e->voidfn(stmts_fn);
    }

    if (call_init) {
        ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol("__main__"))->m_symtab
            ->erase_symbol("__main____lpython_interactive_init_" + std::to_string(eval_count) + "__");
    }
    if (call_stmts) {
        ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol("__main__"))->m_symtab
            ->erase_symbol("__main____lpython_interactive_stmts_" + std::to_string(eval_count) + "__");
    }

    eval_count++;
    return result;
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

Result<LCompilers::LPython::AST::ast_t*> PythonCompiler::get_ast2(
            const std::string &code_orig, diag::Diagnostics &diagnostics)
{
    // Src -> AST
    const std::string *code=&code_orig;
    std::string tmp;
    Result<LCompilers::LPython::AST::ast_t*>
        res = LCompilers::LPython::parse_to_ast(al, *code, 0, diagnostics);
    if (res.ok) {
        return (LCompilers::LPython::AST::ast_t*)res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
}

Result<ASR::TranslationUnit_t*> PythonCompiler::get_asr3(
    LCompilers::LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics,
    LocationManager &lm)
{
    ASR::TranslationUnit_t* asr;
    // AST -> ASR
    if (symbol_table) {
        symbol_table->mark_all_variables_external(al);
    }
    auto res = LCompilers::LPython::python_ast_to_asr(al, lm, symbol_table, ast, diagnostics,
        compiler_options, true, "__main__", "", false, true);
    if (res.ok) {
        asr = res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
    if (!symbol_table) symbol_table = asr->m_symtab;

    return asr;
}

Result<std::unique_ptr<LLVMModule>> PythonCompiler::get_llvm3(
#ifdef HAVE_LFORTRAN_LLVM
    ASR::TranslationUnit_t &asr, LCompilers::PassManager& lpm,
    diag::Diagnostics &diagnostics, const std::string &infile
#else
    ASR::TranslationUnit_t &/*asr*/, LCompilers::PassManager&/*lpm*/,
    diag::Diagnostics &/*diagnostics*/,const std::string &/*infile*/
#endif
    )
{
#ifdef HAVE_LFORTRAN_LLVM
    if (compiler_options.emit_debug_info) {
        if (!compiler_options.emit_debug_line_column) {
            diagnostics.add(LCompilers::diag::Diagnostic(
                "The `emit_debug_line_column` is not enabled; please use the "
                "`--debug-with-line-column` option to get the correct "
                "location information",
                LCompilers::diag::Level::Error,
                LCompilers::diag::Stage::Semantic, {})
            );
            Error err;
            return err;
        }
    }
    // ASR -> LLVM
    std::unique_ptr<LCompilers::LLVMModule> m;
    Result<std::unique_ptr<LCompilers::LLVMModule>> res
        = asr_to_llvm(asr, diagnostics,
            e->get_context(), al, lpm, compiler_options,
            "", infile); // ??? What about run function
    if (res.ok) {
        m = std::move(res.result);
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }

    if (compiler_options.po.fast) {
        e->opt(*m->m_m);
    }

    return m;
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

} // namespace LCompilers::LPython
