#ifndef LCOMPILERS_PYTHON_SERIALIZATION_H
#define LCOMPILERS_PYTHON_SERIALIZATION_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>

namespace LCompilers::LPython {

    AST::ast_t* deserialize_ast(Allocator &al, const std::string &s);

}

#endif // LCOMPILERS_PYTHON_SERIALIZATION_H
