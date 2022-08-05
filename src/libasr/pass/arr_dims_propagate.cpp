#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/for_all.h>
#include <libasr/pass/stmt_walk_visitor.h>

namespace LFortran {

/*
 * This ASR pass replaces ttype for all arrays passed.
 *
 * Converts:
 *      integer :: a(:, :)
 *
 * to:
 *      integer :: a(2, 3)
 */

class ArrDimsPropagate : public ASR::StatementsFirstBaseWalkVisitor<ArrDimsPropagate>
{
private:
//    Allocator &m_al;
public:
    ArrDimsPropagate(Allocator &/*al*/) /*: m_al(al)*/ { }

    void visit_FunctionCall(const ASR::FunctionCall_t &x) {
        ASR::Function_t *fn = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(x.m_name));

        for (size_t i = 0; i < x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value) && ASRUtils::is_array(ASRUtils::expr_type(x.m_args[i].m_value))) {
                ASR::Variable_t* v = ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                ASR::Variable_t *fn_param = ASRUtils::EXPR2VAR(fn->m_args[i]);
                ASR::dimension_t* m_dims;
                int n_dims = ASRUtils::extract_dimensions_from_ttype(fn_param->m_type, m_dims);
                if (n_dims > 0 && !m_dims[0].m_length && ASRUtils::check_equal_type(v->m_type, fn_param->m_type)) {
                    fn_param->m_type = v->m_type;
                }
            }
        }
    }

    void visit_SubroutineCall(const ASR::SubroutineCall_t &x) {
        ASR::Function_t *sb = ASR::down_cast<ASR::Function_t>(ASRUtils::symbol_get_past_external(x.m_name));
        for (size_t i = 0; i < x.n_args; i++) {
            if (ASR::is_a<ASR::Var_t>(*x.m_args[i].m_value) && ASRUtils::is_array(ASRUtils::expr_type(x.m_args[i].m_value))) {
                ASR::Variable_t* v = ASRUtils::EXPR2VAR(x.m_args[i].m_value);
                ASR::Variable_t *sb_param = ASRUtils::EXPR2VAR(sb->m_args[i]);
                ASR::dimension_t* m_dims;
                int n_dims = ASRUtils::extract_dimensions_from_ttype(sb_param->m_type, m_dims);
                if (n_dims > 0 && !m_dims[0].m_length && ASRUtils::check_equal_type(v->m_type, sb_param->m_type)) {
                    sb_param->m_type = v->m_type;
                }
            }
        }
    }
};

void pass_propagate_arr_dims(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrDimsPropagate v(al);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
