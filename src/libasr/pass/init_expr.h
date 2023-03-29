#ifndef LFORTRAN_PASS_INIT_EXPR_H
#define LFORTRAN_PASS_INIT_EXPR_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_init_expr(Allocator &al, ASR::TranslationUnit_t &unit,
                                const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_INIT_EXPR_H
