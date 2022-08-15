#ifndef LFORTRAN_PASS_UPDATE_ARRAY_DIM_H
#define LFORTRAN_PASS_UPDATE_ARRAY_DIM_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_update_array_dim_intrinsic_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                               const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_UPDATE_ARRAY_DIM_H
