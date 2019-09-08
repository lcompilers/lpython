#ifndef LFORTRAN_PICKLE_H
#define LFORTRAN_PICKLE_H

#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

    std::string pickle(LFortran::AST::ast_t &ast);
    std::string pickle(LFortran::ASR::asr_t &asr);

}

#endif // LFORTRAN_PICKLE_H
