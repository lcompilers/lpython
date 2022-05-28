#ifndef LCOMPILERS_PASS_UNUSED_FUNCTIONS_H
#define LCOMPILERS_PASS_UNUSED_FUNCTIONS_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_unused_functions(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_UNUSED_FUNCTIONS_H
