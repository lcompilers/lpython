#ifndef LFORTRAN_PICKLE_H
#define LFORTRAN_PICKLE_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>
#include <libasr/location.h>

namespace LCompilers::LPython {

    // Pickle an ASR node
    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);

    // Print the tree structure
	std::string pickle_tree_python(AST::ast_t &ast, bool colors=true);

    // Print the ASR in json format
    std::string pickle_json(AST::ast_t &ast, LocationManager &lm);
}

#endif // LFORTRAN_PICKLE_H
