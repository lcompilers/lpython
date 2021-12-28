#ifndef LFORTRAN_PICKLE_H
#define LFORTRAN_PICKLE_H

#include <lfortran/parser/parser_stype.h>
#include <lfortran/ast.h>
#include <libasr/asr.h>

namespace LFortran {

    // Pickle a token
    std::string pickle(int token, const YYSTYPE &yystype, bool colors=false);

    // Pickle an AST node
    std::string pickle(AST::ast_t &ast, bool colors=false, bool indent=false);
    std::string pickle(AST::TranslationUnit_t &ast, bool colors=false, bool indent=false);

    // Pickle an ASR node
    std::string pickle(ASR::asr_t &asr, bool colors=false, bool indent=false,
            bool show_intrinsic_modules=false);
    std::string pickle(ASR::TranslationUnit_t &asr, bool colors=false,
            bool indent=false, bool show_intrinsic_modules=false);

}

#endif // LFORTRAN_PICKLE_H
