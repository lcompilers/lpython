#ifndef LFORTRAN_PASS_CLASS_CONSTRUCTOR_H
#define LFORTRAN_PASS_CLASS_CONSTRUCTOR_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_replace_class_constructor(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LFortran

#endif // LFORTRAN_PASS_CLASS_CONSTRUCTOR_H
