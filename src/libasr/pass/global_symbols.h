#ifndef LFORTRAN_PASS_GLOBAL_SYMBOLS_H
#define LFORTRAN_PASS_GLOBAL_SYMBOLS_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_wrap_global_syms_into_module(Allocator &al,
        ASR::TranslationUnit_t &unit,
        const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_GLOBAL_SYMBOLS_H
