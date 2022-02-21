#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LFortran::Python {

    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
        Python::AST::ast_t &ast, diag::Diagnostics &diagnostics, bool main_module);
    Result<AST::ast_t*> parse_python_file(Allocator &al,
            const std::string &runtime_library_dir,
            const std::string &infile);

} // namespace LFortran

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
