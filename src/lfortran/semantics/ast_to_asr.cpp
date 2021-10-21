#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <cmath>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/semantics/asr_implicit_cast_rules.h>
#include <lfortran/semantics/ast_common_visitor.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>


namespace LFortran {

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, AST::TranslationUnit_t &ast,
        diag::Diagnostics &diagnostics,
        SymbolTable *symbol_table);

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        AST::TranslationUnit_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit);

Result<ASR::TranslationUnit_t*> ast_to_asr(Allocator &al,
    AST::TranslationUnit_t &ast, diag::Diagnostics &diagnostics,
    SymbolTable *symbol_table, bool symtab_only)
{
    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, ast, diagnostics, symbol_table);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    if (!symtab_only) {
        auto res = body_visitor(al, ast, diagnostics, unit);
        if (res.ok) {
            tu = res.result;
        } else {
            return res.error;
        }
        LFORTRAN_ASSERT(asr_verify(*tu));
    }
    return tu;
}

} // namespace LFortran
