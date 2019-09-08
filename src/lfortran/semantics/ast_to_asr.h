#ifndef LFORTRAN_AST_TO_ASR_H
#define LFORTRAN_AST_TO_ASR_H

#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

    void ast_to_asr(AST::ast_t &ast, Allocator &al, ASR::asr_t **asr);

} // namespace LFortran

#endif // LFORTRAN_AST_TO_ASR_H
