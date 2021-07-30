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
#include <lfortran/semantics/asr_common_visitor.h>
#include <lfortran/semantics/ast_symboltable_visitor.h>
#include <lfortran/semantics/ast_body_visitor.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>


namespace LFortran {

ASR::TranslationUnit_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast,
        SymbolTable *symbol_table)
{
    SymbolTableVisitor v(al, symbol_table);
    v.visit_TranslationUnit(ast);
    ASR::asr_t *unit = v.asr;

    // Uncomment for debugging the ASR after SymbolTable building:
    // std::cout << pickle(*unit) << std::endl;

    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));
    return tu;
}

} // namespace LFortran
