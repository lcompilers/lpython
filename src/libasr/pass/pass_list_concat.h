#ifndef LIBASR_PASS_LIST_CONCAT_H
#define LIBASR_PASS_LIST_CONCAT_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LFortran {

    void pass_list_concat(Allocator &al, ASR::TranslationUnit_t &unit,
                        const LCompilers::PassOptions& /*pass_options*/);

} // namespace LFortran

#endif // LIBASR_PASS_LIST_CONCAT_H
