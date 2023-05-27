#ifndef LFORTRAN_PASS_WHERE_H
#define LFORTRAN_PASS_WHERE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_where(Allocator &al, ASR::TranslationUnit_t &unit, const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_WHERE_H
