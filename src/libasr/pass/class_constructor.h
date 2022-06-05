#ifndef LCOMPILERS_PASS_CLASS_CONSTRUCTOR_H
#define LCOMPILERS_PASS_CLASS_CONSTRUCTOR_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_class_constructor(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_CLASS_CONSTRUCTOR_H
