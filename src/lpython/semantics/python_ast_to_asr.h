#ifndef LCOMPILERS_PYTHON_AST_TO_ASR_H
#define LCOMPILERS_PYTHON_AST_TO_ASR_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LCompilers::LPython {

    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    std::string pickle_tree_python(AST::ast_t &ast, bool colors=true);
    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
        LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics, bool main_module,
        bool symtab_only);

} // namespace LCompilers

#endif // LCOMPILERS_PYTHON_AST_TO_ASR_H
