#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LCompilers::LPython {

    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al, LocationManager &lm, SymbolTable* symtab,
        LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics, CompilerOptions &compiler_options,
            bool main_module, std::string module_name, std::string file_path, bool allow_implicit_casting=false, size_t eval_count=0);

    int save_pyc_files(const ASR::TranslationUnit_t &u,
                       std::string infile, LocationManager& lm);

} // namespace LCompilers::LPython

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
