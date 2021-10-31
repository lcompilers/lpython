#ifndef LFORTRAN_PASS_FOR_ALL
#define LFORTRAN_PASS_FOR_ALL

#include <lfortran/asr.h>

namespace LFortran {

    void pass_replace_forall(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_FOR_ALL
