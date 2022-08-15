#ifndef LFORTRAN_PASS_ARRAY_OP_H
#define LFORTRAN_PASS_ARRAY_OP_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
                               const LCompilers::PassOptions& pass_options);

} // namespace LFortran

#endif // LFORTRAN_PASS_ARRAY_OP_H
