#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lfortran/python_ast.h>
#include <lfortran/asr.h>

namespace LFortran::Python {

    Result<ASR::TranslationUnit_t*> ast_to_asr(Allocator &al,
        Python::AST::Module_t &ast, diag::Diagnostics &diagnostics,
        SymbolTable *symbol_table=nullptr,
        bool symtab_only=false);

} // namespace LFortran

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
