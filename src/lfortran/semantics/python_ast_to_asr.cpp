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
    Python::AST::ast_t &ast, diag::Diagnostics &diagnostics)
{
    Error error;
    diagnostics.diagnostics.push_back(
            diag::Diagnostic("start", diag::Level::Error, diag::Stage::Semantic, {})
    );
    return error;

    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, ast, diagnostics);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    auto res2 = body_visitor(al, ast, diagnostics, unit);
    if (res2.ok) {
        tu = res2.result;
    } else {
        return res2.error;
    }
    LFORTRAN_ASSERT(asr_verify(*tu));

    return tu;
}

} // namespace LFortran
