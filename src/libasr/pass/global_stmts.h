#ifndef LFORTRAN_PASS_GLOBAL_STMTS_H
#define LFORTRAN_PASS_GLOBAL_STMTS_H

#include <lfortran/asr.h>

namespace LFortran {

    void pass_wrap_global_stmts_into_function(Allocator &al,
                ASR::TranslationUnit_t &unit, const std::string &fn_name_s);

} // namespace LFortran

#endif // LFORTRAN_PASS_GLOBAL_STMTS_H
