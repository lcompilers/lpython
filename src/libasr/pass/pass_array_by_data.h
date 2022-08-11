#ifndef LFORTRAN_PASS_ARRAY_BY_DATA_H
#define LFORTRAN_PASS_ARRAY_BY_DATA_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_ARRAY_BY_DATA_H
