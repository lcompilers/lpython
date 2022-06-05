#ifndef LCOMPILERS_PASS_DO_LOOPS_H
#define LCOMPILERS_PASS_DO_LOOPS_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_do_loops(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_DO_LOOPS_H
