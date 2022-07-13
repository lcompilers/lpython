#ifndef LFORTRAN_PYTHON_AST_TO_ASR_H
#define LFORTRAN_PYTHON_AST_TO_ASR_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LFortran::LPython {
    struct lsp_locations {
    std::string symbol_name;
    uint32_t first_line;
    uint32_t first_column;
    uint32_t last_line;
    uint32_t last_column;
    };
    std::vector<lsp_locations> get_symbol_locations(Allocator &al, LFortran::LPython::AST::ast_t &ast,
        LFortran::diag::Diagnostics &diagnostics, bool main_module,
        std::string file_path, const LFortran::LocationManager &lm);
    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    std::string pickle_tree_python(AST::ast_t &ast, bool colors=true);
    Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
        LPython::AST::ast_t &ast, diag::Diagnostics &diagnostics,
        bool main_module, bool disable_main, bool symtab_only, std::string file_path);

} // namespace LFortran

#endif // LFORTRAN_PYTHON_AST_TO_ASR_H
