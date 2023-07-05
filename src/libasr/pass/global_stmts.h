#ifndef LIBASR_PASS_GLOBAL_STMTS_H
#define LIBASR_PASS_GLOBAL_STMTS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_wrap_global_stmts(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_GLOBAL_STMTS_H
