#ifndef LFORTRAN_PASS_ARR_DIMS_PROPAGATE
#define LFORTRAN_PASS_ARR_DIMS_PROPAGATE

#include <libasr/asr.h>

namespace LFortran {

    void pass_propagate_arr_dims(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_ARR_DIMS_PROPAGATE
