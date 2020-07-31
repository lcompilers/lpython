#ifndef LFORTRAN_AST_TO_CPP_HIP_H
#define LFORTRAN_AST_TO_CPP_HIP_H

#include <lfortran/ast.h>

namespace LFortran {

    // Converts AST to C++ source code with parallelization using HIP
    std::string ast_to_cpp_hip(LFortran::AST::ast_t &ast);

}

#endif // LFORTRAN_AST_TO_CPP_HIP_H
