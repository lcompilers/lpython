#ifndef LFORTRAN_PASS_SELECT_CASE_H
#define LFORTRAN_PASS_SELECT_CASE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_select_case(Allocator &al, ASR::TranslationUnit_t &unit,
                                  const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_SELECT_CASE_H
