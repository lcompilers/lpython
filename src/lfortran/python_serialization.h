#ifndef LFORTRAN_PYTHON_SERIALIZATION_H
#define LFORTRAN_PYTHON_SERIALIZATION_H

#include <lfortran/python_ast.h>
#include <lfortran/asr.h>

namespace LFortran::Python {

    AST::ast_t* deserialize_ast(Allocator &al, const std::string &s);

}

#endif // LFORTRAN_PYTHON_SERIALIZATION_H
