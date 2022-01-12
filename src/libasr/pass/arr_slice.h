#ifndef LFORTRAN_PASS_ARR_SLICE_H
#define LFORTRAN_PASS_ARR_SLICE_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path);

} // namespace LFortran

#endif // LFORTRAN_PASS_ARR_SLICE_H
