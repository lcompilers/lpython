#ifndef LIBASR_PASS_INIT_EXPR_H
#define LIBASR_PASS_INIT_EXPR_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_init_expr(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_INIT_EXPR_H
