#ifndef LIBASR_PASS_GLOBAL_STMTS_PROGRAM_H
#define LIBASR_PASS_GLOBAL_STMTS_PROGRAM_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_wrap_global_stmts_into_program(Allocator &al, ASR::TranslationUnit_t &unit,
                                             const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_GLOBAL_STMTS_PROGRAM_H
