#ifndef LIBASR_PASS_REPLACE_PRINT_LIST_TUPLE_H
#define LIBASR_PASS_REPLACE_PRINT_LIST_TUPLE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_print_list_tuple(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_REPLACE_PRINT_LIST_TUPLE_H
