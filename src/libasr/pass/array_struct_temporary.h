#ifndef LIBASR_PASS_ARRAY_STRUCT_TEMPORARY_H
#define LIBASR_PASS_ARRAY_STRUCT_TEMPORARY_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_array_struct_temporary(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_ARRAY_STRUCT_TEMPORARY_H
