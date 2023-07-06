#ifndef LIBASR_PASS_REPLACE_PRINT_ARR_H
#define LIBASR_PASS_REPLACE_PRINT_ARR_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_print_arr(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_PRINT_ARR_H
