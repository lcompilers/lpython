#ifndef LFORTRAN_PASS_UNUSED_FUNCTIONS_H
#define LFORTRAN_PASS_UNUSED_FUNCTIONS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_unused_functions(Allocator &al, ASR::TranslationUnit_t &unit,
                               const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_UNUSED_FUNCTIONS_H
