#ifndef LCOMPILERS_PASS_ARRAY_OP_H
#define LCOMPILERS_PASS_ARRAY_OP_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_ARRAY_OP_H
