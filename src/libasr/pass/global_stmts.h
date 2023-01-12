#ifndef LFORTRAN_PASS_GLOBAL_STMTS_H
#define LFORTRAN_PASS_GLOBAL_STMTS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_wrap_global_stmts_into_function(Allocator &al, ASR::TranslationUnit_t &unit,
                                              const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_GLOBAL_STMTS_H
