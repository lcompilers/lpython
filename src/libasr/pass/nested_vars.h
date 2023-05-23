#ifndef LFORTRAN_PASS_NESTED_VARS_H
#define LFORTRAN_PASS_NESTED_VARS_H

#include <libasr/asr.h>

namespace LCompilers {

     void pass_nested_vars(Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_NESTED_VARS_H
