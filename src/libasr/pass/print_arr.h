#ifndef LCOMPILERS_PASS_PRINT_ARR_H
#define LCOMPILERS_PASS_PRINT_ARR_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_replace_print_arr(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_PRINT_ARR_H
