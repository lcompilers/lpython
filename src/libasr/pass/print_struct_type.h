#ifndef LIBASR_PASS_PRINT_STRUCT_TYPE_H
#define LIBASR_PASS_PRINT_STRUCT_TYPE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_print_struct_type(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_PRINT_STRUCT_TYPE_H
