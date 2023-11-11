#ifndef LIBASR_PASS_INSERT_DEALLOCATE_H
#define LIBASR_PASS_INSERT_DEALLOCATE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_insert_deallocate(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_INSERT_DEALLOCATE_H
