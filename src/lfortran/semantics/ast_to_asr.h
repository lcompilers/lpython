#ifndef LFORTRAN_AST_TO_ASR_H
#define LFORTRAN_AST_TO_ASR_H

#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

    ASR::asr_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast);

} // namespace LFortran

#endif // LFORTRAN_AST_TO_ASR_H
