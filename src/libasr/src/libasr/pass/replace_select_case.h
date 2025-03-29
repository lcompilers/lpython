#ifndef LIBASR_PASS_REPLACE_SELECT_CASE_H
#define LIBASR_PASS_REPLACE_SELECT_CASE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_SELECT_CASE_H
