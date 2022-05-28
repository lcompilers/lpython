#ifndef LCOMPILERS_PASS_GLOBAL_STMTS_H
#define LCOMPILERS_PASS_GLOBAL_STMTS_H

#include <libasr/asr.h>

namespace LCompilers {

    void pass_wrap_global_stmts_into_function(Allocator &al,
                ASR::TranslationUnit_t &unit, const std::string &fn_name_s);

} // namespace LCompilers

#endif // LCOMPILERS_PASS_GLOBAL_STMTS_H
