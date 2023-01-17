#ifndef LFORTRAN_PASS_PARAM_TO_CONST_H
#define LFORTRAN_PASS_PARAM_TO_CONST_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_param_to_const(Allocator &al, ASR::TranslationUnit_t &unit,
                                     const LCompilers::PassOptions& pass_options
                                     );

} // namespace LCompilers

#endif // LFORTRAN_PASS_PARAM_TO_CONST_H
