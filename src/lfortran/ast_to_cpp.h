#ifndef LFORTRAN_AST_TO_CPP_H
#define LFORTRAN_AST_TO_CPP_H

#include <lfortran/ast.h>

namespace LFortran {

    // Converts AST to C++ source code
    std::string ast_to_cpp(LFortran::AST::ast_t &ast);

}

#endif // LFORTRAN_AST_TO_CPP_H
