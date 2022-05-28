#ifndef LCOMPILERS_PASS_ARR_SLICE_H
#define LCOMPILERS_PASS_ARR_SLICE_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_arr_slice(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_ARR_SLICE_H
