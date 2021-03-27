#ifndef LFORTRAN_SERIALIZATION_H
#define LFORTRAN_SERIALIZATION_H

#include <lfortran/parser/parser_stype.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

    std::string serialize(AST::ast_t &ast);
    std::string serialize(AST::TranslationUnit_t &unit);

    AST::ast_t* deserialize(Allocator &al, const std::string &s);

    std::string uint64_to_string(uint64_t i);
    uint64_t string_to_uint64(const std::string &s);
    uint64_t string_to_uint64(const char *s);
}

#endif // LFORTRAN_SERIALIZATION_H
