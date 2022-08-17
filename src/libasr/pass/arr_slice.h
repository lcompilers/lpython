#ifndef LFORTRAN_PASS_ARR_SLICE_H
#define LFORTRAN_PASS_ARR_SLICE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
                                const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_ARR_SLICE_H
