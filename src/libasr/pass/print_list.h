#ifndef LFORTRAN_PASS_PRINT_LIST_H
#define LFORTRAN_PASS_PRINT_LIST_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_print_list(
    Allocator &al, ASR::TranslationUnit_t &unit,
    const LCompilers::PassOptions &pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_PRINT_LIST_H
