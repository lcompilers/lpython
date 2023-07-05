#ifndef LIBASR_PASS_GLOBAL_SYMBOLS_H
#define LIBASR_PASS_GLOBAL_SYMBOLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_wrap_global_symbols(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_GLOBAL_SYMBOLS_H
