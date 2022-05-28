#ifndef LCOMPILERS_PASS_FOR_ALL
#define LCOMPILERS_PASS_FOR_ALL

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_forall(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_FOR_ALL
