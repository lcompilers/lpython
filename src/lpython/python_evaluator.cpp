#include <iostream>
#include <fstream>
#include <string>

#include <lpython/python_evaluator.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/python_ast.h>
#include <lpython/pickle.h>
#include <lpython/parser/parser.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr.h>
#include <libasr/asr_scopes.h>

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
#endif
    eval_count{1},
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
    Result<ASR::TranslationUnit_t*> res2 = get_asr3(*ast, diagnostics, lm, true);
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
    std::string module_prefix = "__module___main___";
    std::string module_name = "__main__";
    std::string sym_name = module_name + "global_stmts_" + std::to_string(eval_count) + "__";
    run_fn = module_prefix + sym_name;

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

    bool call_run_fn = false;
    if (m->get_return_type(run_fn) != "none") {
        call_run_fn = true;
    }

    e->add_module(std::move(m));
    if (call_run_fn) {
        e->voidfn(run_fn);
    }

    if (call_run_fn) {
        ASR::down_cast<ASR::Module_t>(symbol_table->resolve_symbol(module_name))->m_symtab
            ->erase_symbol(sym_name);
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
    Result<LCompilers::LPython::AST::Module_t*>
        res = LCompilers::LPython::parse(al, *code, 0, diagnostics);
    if (res.ok) {
        return (LCompilers::LPython::AST::ast_t*)res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return res.error;
    }
}

Result<ASR::TranslationUnit_t*> PythonCompiler::get_asr3(
    LCompilers::LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics,
    LocationManager &lm, bool is_interactive)
{
    ASR::TranslationUnit_t* asr;
    // AST -> ASR
    if (symbol_table) {
        symbol_table->mark_all_variables_external(al);
    }
    auto res = LCompilers::LPython::python_ast_to_asr(al, lm, symbol_table, ast, diagnostics,
        compiler_options, true, "__main__", "", false, is_interactive ? eval_count : 0);
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
            run_fn, infile);
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
