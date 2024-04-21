#ifndef LFORTRAN_CASTING_UTILS_H
#define LFORTRAN_CASTING_UTILS_H


#include <libasr/asr.h>

namespace LCompilers::CastingUtil {

    int get_type_priority(ASR::ttypeType type);

    int get_src_dest(ASR::expr_t* left_expr, ASR::expr_t* right_expr,
                      ASR::expr_t*& src_expr, ASR::expr_t*& dest_expr,
                      ASR::ttype_t*& src_type, ASR::ttype_t*& dest_type,
                      bool is_assign, bool allow_int_to_float=false);

    ASR::expr_t* perform_casting(ASR::expr_t* expr, ASR::ttype_t* dest,
                                Allocator& al, const Location& loc);
}

#endif // LFORTRAN_CASTING_UTILS_H
