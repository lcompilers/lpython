#ifndef LIBASR_PASS_REPLACE_SYMBOLIC_H
#define LIBASR_PASS_REPLACE_SYMBOLIC_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_symbolic(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_SYMBOLIC_H
