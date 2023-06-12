#ifndef LFORTRAN_PASS_PRINT_STRUCT_TYPE_H
#define LFORTRAN_PASS_PRINT_STRUCT_TYPE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_print_struct_type(
        Allocator &al, ASR::TranslationUnit_t &unit,
        const LCompilers::PassOptions& pass_options);

} // namespace LCompilers

#endif // LFORTRAN_PASS_PRINT_STRUCT_TYPE_H
