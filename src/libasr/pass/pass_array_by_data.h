#ifndef LIBASR_PASS_ARRAY_BY_DATA_H
#define LIBASR_PASS_ARRAY_BY_DATA_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_array_by_data(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_ARRAY_BY_DATA_H
