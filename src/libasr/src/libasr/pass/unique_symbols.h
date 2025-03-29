#ifndef LIBASR_PASS_UNIQUE_SYMBOLS_H
#define LIBASR_PASS_UNIQUE_SYMBOLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_unique_symbols(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_UNIQUE_SYMBOLS_H
