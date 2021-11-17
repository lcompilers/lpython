#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <cmath>

#include <lfortran/python_ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/semantics/python_ast_to_asr.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>


namespace LFortran::Python {

Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
    Python::AST::ast_t &ast, diag::Diagnostics &diagnostics,
    bool symtab_only)
{
    ASR::TranslationUnit_t *tu = nullptr;
    //return tu;

    Error error;
    diagnostics.diagnostics.push_back(
            diag::Diagnostic("start", diag::Level::Error, diag::Stage::Semantic, {})
    );
    return error;
}

} // namespace LFortran
