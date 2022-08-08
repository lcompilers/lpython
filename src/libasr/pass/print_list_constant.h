#ifndef LFORTRAN_PASS_PRINT_LIST_CONSTANT_H
#define LFORTRAN_PASS_PRINT_LIST_CONSTANT_H

#include <libasr/asr.h>

namespace LFortran {

    void pass_replace_print_list_constant(Allocator &al, ASR::TranslationUnit_t &unit,
        const std::string &rl_path);

} // namespace LFortran

#endif // LFORTRAN_PASS_PRINT_LIST_CONSTANT_H
