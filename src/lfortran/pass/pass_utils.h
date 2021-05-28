#ifndef LFORTRAN_PASS_UTILS_H
#define LFORTRAN_PASS_UTILS_H

#include <lfortran/asr.h>
#include <lfortran/containers.h>

namespace LFortran {

    namespace PassUtils {

        struct dimension_descriptor
        {
            int lbound, ubound;
        };

        bool is_array(ASR::expr_t* x, Allocator& al);

        void get_dims(ASR::expr_t* x, Vec<dimension_descriptor>& result, Allocator& al);

        ASR::expr_t* create_array_ref(ASR::expr_t* arr_expr, Vec<ASR::expr_t*>& idx_vars, Allocator& al);

        ASR::expr_t* create_array_ref(ASR::symbol_t* arr, Vec<ASR::expr_t*>& idx_vars, Allocator& al,
                                      const Location& loc, ASR::ttype_t* _type);

        void create_idx_vars(Vec<ASR::expr_t*>& idx_vars, int n_dims, const Location& loc, Allocator& al, ASR::TranslationUnit_t& unit, std::string suffix="_k");

    }

} // namespace LFortran

#endif // LFORTRAN_PASS_UTILS_H
