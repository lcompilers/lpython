#ifndef LIBASR_PASS_CLASS_CONSTRUCTOR_H
#define LIBASR_PASS_CLASS_CONSTRUCTOR_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    void pass_replace_class_constructor(Allocator &al, ASR::TranslationUnit_t &unit,
                                const PassOptions &pass_options);

} // namespace LCompilers

#endif // LIBASR_PASS_CLASS_CONSTRUCTOR_H
