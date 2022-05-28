#ifndef LCOMPILERS_PASS_PARAM_TO_CONST_H
#define LCOMPILERS_PASS_PARAM_TO_CONST_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_param_to_const(Allocator &al, ASR::TranslationUnit_t &unit);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_PARAM_TO_CONST_H
