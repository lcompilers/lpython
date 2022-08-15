#ifndef LFORTRAN_PASS_FOR_ALL
#define LFORTRAN_PASS_FOR_ALL

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_forall(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_FOR_ALL
