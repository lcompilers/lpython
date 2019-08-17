#ifndef LFORTRAN_AST_TO_JSON_H
#define LFORTRAN_AST_TO_JSON_H

#include <lfortran/ast.h>

namespace LFortran {

    std::string ast_to_json(LFortran::AST::ast_t &ast);

}

#endif // LFORTRAN_AST_TO_JSON_H
