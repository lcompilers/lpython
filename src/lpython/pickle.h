#ifndef LPYTHON_PICKLE_H
#define LPYTHON_PICKLE_H

#include <lpython/python_ast.h>
#include <libasr/asr.h>
#include <libasr/location.h>

namespace LCompilers::LPython {

    // Pickle an ASR node
    std::string pickle_python(AST::ast_t &ast, bool colors=false, bool indent=false);
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

    // Print the tree structure
	std::string pickle_tree_python(AST::ast_t &ast, bool colors=true);
    std::string pickle_tree(ASR::asr_t &asr, bool colors, bool show_intrinsic_modules);
    std::string pickle_tree(ASR::TranslationUnit_t &asr, bool colors, bool show_intrinsic_modules);

    std::string pickle_json(AST::ast_t &ast, LocationManager &lm);
    std::string pickle_json(ASR::asr_t &asr, LocationManager &lm, bool show_intrinsic_modules);
    std::string pickle_json(ASR::TranslationUnit_t &asr, LocationManager &lm, bool show_intrinsic_modules);

}

#endif // LPYTHON_PICKLE_H
