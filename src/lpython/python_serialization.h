#ifndef LFORTRAN_PYTHON_SERIALIZATION_H
#define LFORTRAN_PYTHON_SERIALIZATION_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LFortran::Python {

    AST::ast_t* deserialize_ast(Allocator &al, const std::string &s);

}

#endif // LFORTRAN_PYTHON_SERIALIZATION_H
