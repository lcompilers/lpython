#ifndef LFORTRAN_SERIALIZATION_H
#define LFORTRAN_SERIALIZATION_H

#include <lfortran/ast.h>
#include <libasr/asr.h>

namespace LFortran {

    std::string serialize(const AST::ast_t &ast);
    std::string serialize(const AST::TranslationUnit_t &unit);
    AST::ast_t* deserialize_ast(Allocator &al, const std::string &s);

}

#endif // LFORTRAN_SERIALIZATION_H
