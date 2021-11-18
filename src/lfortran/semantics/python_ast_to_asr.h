#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lfortran/python_ast.h>
#include <lfortran/asr.h>

namespace LFortran::Python {

    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
        Python::AST::ast_t &ast, diag::Diagnostics &diagnostics);

} // namespace LFortran

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
