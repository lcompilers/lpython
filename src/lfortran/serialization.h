#ifndef LFORTRAN_SERIALIZATION_H
#define LFORTRAN_SERIALIZATION_H

#include <lfortran/parser/parser_stype.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

    std::string serialize(const AST::ast_t &ast);
    std::string serialize(const AST::TranslationUnit_t &unit);
    AST::ast_t* deserialize_ast(Allocator &al, const std::string &s);

    std::string serialize(const ASR::asr_t &asr);
    std::string serialize(const ASR::TranslationUnit_t &unit);
    ASR::asr_t* deserialize_asr(Allocator &al, const std::string &s,
            bool load_symtab_id, SymbolTable &symtab);

    void fix_external_symbols(ASR::TranslationUnit_t &unit,
            SymbolTable &external_symtab);
}

#endif // LFORTRAN_SERIALIZATION_H
