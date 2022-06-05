#ifndef LCOMPILERS_PASS_SELECT_CASE_H
#define LCOMPILERS_PASS_SELECT_CASE_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_SELECT_CASE_H
