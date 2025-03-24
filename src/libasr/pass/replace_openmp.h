#ifndef LIBASR_PASS_REPLACE_OPENMP
#define LIBASR_PASS_REPLACE_OPENMP

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_openmp(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_OPENMP
