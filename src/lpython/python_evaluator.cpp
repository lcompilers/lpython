#include <iostream>
#include <fstream>

#include <lpython/python_evaluator.h>
#include <libasr/codegen/asr_to_cpp.h>
#include <libasr/exception.h>
#include <libasr/asr.h>

#ifdef HAVE_LFORTRAN_LLVM
#include <libasr/codegen/evaluator.h>
#include <libasr/codegen/asr_to_llvm.h>
#else
namespace LFortran {
    class LLVMEvaluator {};
}
#endif

namespace LFortran {


/* ------------------------------------------------------------------------- */
// PythonEvaluator

PythonCompiler::PythonCompiler(CompilerOptions compiler_options)
    :
    al{1024*1024},
#ifdef HAVE_LFORTRAN_LLVM
    e{std::make_unique<LLVMEvaluator>()},
    eval_count{0},
#endif
    compiler_options{compiler_options}
//    symbol_table{nullptr}
{
}

PythonCompiler::~PythonCompiler() = default;


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
    eval_count++;
    run_fn = "__lfortran_evaluate_" + std::to_string(eval_count);

    // ASR -> LLVM
    std::unique_ptr<LFortran::LLVMModule> m;
    Result<std::unique_ptr<LFortran::LLVMModule>> res
        = asr_to_llvm(asr, diagnostics,
            e->get_context(), al, lpm, compiler_options,
            run_fn, infile);
    if (res.ok) {
        m = std::move(res.result);
    } else {
        LFORTRAN_ASSERT(diagnostics.has_error())
        return res.error;
    }

    if (compiler_options.fast) {
        e->opt(*m->m_m);
    }

    return m;
#else
    throw LCompilersException("LLVM is not enabled");
#endif
}

} // namespace LFortran
