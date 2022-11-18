#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/update_array_dim_intrinsic_calls.h>

#include <vector>
#include <utility>


namespace LFortran {

/*

The following class implements replacer methods for replacing
calls to intrinsic functions (which provide dimensional information
of arrays like size) with the actual result. This happens only for arrays
which are input arguments to a procedure, and the dimensional information
in their type is not empty. See the example below,

subrotuine copy(x, y, n)
    integer, intent(in) :: n
    integer, intent(in) :: x(n)
    integer, intent(in) :: y(n)
    do i = 1, size(x)
        do j  = 1, size(y)
            print *, x(i) + y(j)
        end do
    end do
end subroutine

converted to,

subrotuine copy(x, y, n)
    integer, intent(in) :: n
    integer, intent(in) :: x(n)
    integer, intent(in) :: y(n)
    do i = 1, n
        do j  = 1, n
            print *, x(i) + y(j)
        end do
    end do
end subroutine

*/
class ReplaceArrayDimIntrinsicCalls: public ASR::BaseExprReplacer<ReplaceArrayDimIntrinsicCalls> {

    private:

    Allocator& al;

    public:

    ReplaceArrayDimIntrinsicCalls(Allocator& al_) : al(al_)
    {}

    void replace_ArraySize(ASR::ArraySize_t* x) {
        if( !ASR::is_a<ASR::Var_t>(*x->m_v) ||
            (x->m_dim != nullptr && !ASRUtils::is_value_constant(x->m_dim)) ) {
            return ;
        }

        ASR::Variable_t* v = ASRUtils::EXPR2VAR(x->m_v);
        ASR::ttype_t* array_type = ASRUtils::expr_type(x->m_v);
        ASR::dimension_t* dims = nullptr;
        int n = ASRUtils::extract_dimensions_from_ttype(array_type, dims);
        bool is_argument = v->m_intent == ASRUtils::intent_in || v->m_intent == ASRUtils::intent_out;
        if( !(n > 0 && is_argument &&
              !ASRUtils::is_dimension_empty(dims, n)) ) {
            return ;
        }
        if( x->m_dim != nullptr ) {
            int64_t dim = -1;
            ASRUtils::extract_value(x->m_dim, dim);
            *current_expr = dims[dim - 1].m_length;
            return ;
        }

        ASR::expr_t* array_size = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                                    al, x->base.base.loc, 1, x->m_type));
        for( int i = 0; i < n; i++ ) {
            array_size = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc,
                            array_size, ASR::binopType::Mul, dims[i].m_length, x->m_type,
                            nullptr));
        }
        *current_expr = array_size;
    }

    void replace_ArrayBound(ASR::ArrayBound_t* x) {
        if( !ASR::is_a<ASR::Var_t>(*x->m_v) ||
            !ASRUtils::is_value_constant(x->m_dim) ) {
            return ;
        }

        ASR::Variable_t* v = ASRUtils::EXPR2VAR(x->m_v);
        ASR::ttype_t* array_type = ASRUtils::expr_type(x->m_v);
        ASR::dimension_t* dims = nullptr;
        int n = ASRUtils::extract_dimensions_from_ttype(array_type, dims);
        bool is_argument = ASRUtils::is_arg_dummy(v->m_intent);
        if( !(n > 0 && is_argument &&
              !ASRUtils::is_dimension_empty(dims, n)) ) {
            return ;
        }
        int64_t dim = -1;
        ASRUtils::extract_value(x->m_dim, dim);
        if( x->m_bound == ASR::arrayboundType::LBound ) {
            *current_expr = dims[dim - 1].m_start;
        } else {
            ASR::expr_t* ub = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al,
                                x->base.base.loc, dims[dim - 1].m_length,
                                ASR::binopType::Add, dims[dim - 1].m_start,
                                x->m_type, nullptr));
            ASR::expr_t* const_1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                        x->base.base.loc, 1, x->m_type));
            *current_expr = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, x->base.base.loc,
                                ub, ASR::binopType::Sub, const_1, x->m_type, nullptr));
        }
    }

};

/*
This class implements call_replacer method which calls the
ReplaceArrayDimIntrinsicCalls.replace_expr method on all the expressions
present in the ASR.
*/
class ArrayDimIntrinsicCallsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArrayDimIntrinsicCallsVisitor>
{
    private:

        ReplaceArrayDimIntrinsicCalls replacer;

    public:

        ArrayDimIntrinsicCallsVisitor(Allocator& al_) : replacer(al_) {}

        void call_replacer() {
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }

};

void pass_update_array_dim_intrinsic_calls(Allocator &al, ASR::TranslationUnit_t &unit,
                                           const LCompilers::PassOptions& /*pass_options*/) {
    ArrayDimIntrinsicCallsVisitor v(al);
    v.visit_TranslationUnit(unit);
}

} // namespace LFortran
