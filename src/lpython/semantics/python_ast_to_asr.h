#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LCompilers::LPython {

    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    std::string pickle_tree_python(AST::ast_t &ast, bool colors=true);
    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al, LocationManager &lm,
        LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics, CompilerOptions &compiler_options,
            bool main_module, std::string file_path, bool allow_implicit_casting=false);

    int save_pyc_files(const ASR::TranslationUnit_t &u,
                       std::string infile);

} // namespace LCompilers::LPython

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
