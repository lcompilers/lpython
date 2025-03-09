#ifndef LIBASR_PASS_REPLACE_WITH_COMPILE_TIME_VALUES_H
#define LIBASR_PASS_REPLACE_WITH_COMPILE_TIME_VALUES_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_with_compile_time_values(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_WITH_COMPILE_TIME_VALUES_H
