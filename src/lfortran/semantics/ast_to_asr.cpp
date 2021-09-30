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

ASR::asr_t *symbol_table_visitor(Allocator &al, AST::TranslationUnit_t &ast,
        SymbolTable *symbol_table);

ASR::TranslationUnit_t *body_visitor(Allocator &al,
        AST::TranslationUnit_t &ast, ASR::asr_t *unit);

ASR::TranslationUnit_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast,
        SymbolTable *symbol_table, bool symtab_only)
{
    ASR::asr_t *unit = symbol_table_visitor(al, ast, symbol_table);
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    if (!symtab_only) {
        tu = body_visitor(al, ast, unit);
        LFORTRAN_ASSERT(asr_verify(*tu));
    }
    return tu;
}

} // namespace LFortran
