#ifndef LIBASR_PASS_COMPARE_H
#define LIBASR_PASS_COMPARE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_compare(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/);

} // namespace LCompilers

#endif // LIBASR_PASS_COMPARE_H
