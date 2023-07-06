#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_fma.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass converts fused multiplication and addition (FMA) expressions
with a call to internal optimization routine for FMA. This allows
backend specific code generation for FMA expressions resulting in optimized
code.

Converts:

    real :: a, b, c, d
    d = a + b * c

to:

    d = fma(a, b, c)

*/
class FMAVisitor : public PassUtils::SkipOptimizationFunctionVisitor<FMAVisitor>
{
private:
    ASR::TranslationUnit_t &unit;

    LCompilers::PassOptions pass_options;

    ASR::expr_t* fma_var;

    // To make sure that FMA is applied only for
    // the nodes implemented in this class
    bool from_fma;

public:
    FMAVisitor(Allocator &al_, ASR::TranslationUnit_t &unit_,
               const LCompilers::PassOptions& pass_options_) : SkipOptimizationFunctionVisitor(al_),
    unit(unit_), pass_options(pass_options_), fma_var(nullptr), from_fma(false)
    {
        pass_result.reserve(al, 1);
    }

    bool is_BinOpMul(ASR::expr_t* expr) {
        if (ASR::is_a<ASR::RealBinOp_t>(*expr)) {
            ASR::RealBinOp_t* expr_binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
            return expr_binop->m_op == ASR::binopType::Mul;
        }
        return false;
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t& /*x*/) { }
    void visit_ComplexBinOp(const ASR::ComplexBinOp_t& /*x*/) { }
    void visit_LogicalBinOp(const ASR::LogicalBinOp_t& /*x*/) { }

    void visit_RealBinOp(const ASR::RealBinOp_t& x_const) {
        if( !from_fma ) {
            return ;
        }

        from_fma = true;
        LCOMPILERS_ASSERT(ASRUtils::is_real(*x_const.m_type))
        ASR::RealBinOp_t& x = const_cast<ASR::RealBinOp_t&>(x_const);

        fma_var = nullptr;
        visit_expr(*x.m_left);
        if( fma_var ) {
            x.m_left = fma_var;
        }

        fma_var = nullptr;
        visit_expr(*x.m_right);
        if( fma_var ) {
            x.m_right = fma_var;
        }
        fma_var = nullptr;

        if( x.m_op != ASR::binopType::Add && x.m_op != ASR::binopType::Sub ) {
            return ;
        }
        ASR::expr_t *mul_expr = nullptr, *other_expr = nullptr;
        bool is_mul_expr_negative = false, is_other_expr_negative = false;
        if( is_BinOpMul(x.m_right) ) {
            mul_expr = x.m_right;
            other_expr = x.m_left;
            is_mul_expr_negative = (x.m_op == ASR::binopType::Sub);
        } else if( is_BinOpMul(x.m_left) ) {
            mul_expr = x.m_left;
            other_expr = x.m_right;
            is_other_expr_negative = (x.m_op == ASR::binopType::Sub);
        } else {
            return ;
        }

        if( is_other_expr_negative ) {
            other_expr = ASRUtils::EXPR(ASR::make_RealUnaryMinus_t(al, other_expr->base.loc, other_expr,
                ASRUtils::expr_type(other_expr), nullptr));
        }

        ASR::RealBinOp_t* mul_binop = ASR::down_cast<ASR::RealBinOp_t>(mul_expr);
        ASR::expr_t *first_arg = mul_binop->m_left, *second_arg = mul_binop->m_right;

        if( is_mul_expr_negative ) {
            first_arg = ASRUtils::EXPR(ASR::make_RealUnaryMinus_t(al, first_arg->base.loc, first_arg,
                ASRUtils::expr_type(first_arg), nullptr));
        }

        fma_var = PassUtils::get_fma(other_expr, first_arg, second_arg,
                                     al, unit, pass_options, current_scope, x.base.base.loc,
                                     [&](const std::string &msg, const Location &) { throw LCompilersException(msg); });
        from_fma = false;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        from_fma = true;
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        fma_var = nullptr;
        visit_expr(*x.m_value);
        if( fma_var ) {
            xx.m_value = fma_var;
        }
        fma_var = nullptr;
        from_fma = false;
    }

    void visit_IntegerUnaryMinus(const ASR::IntegerUnaryMinus_t &x) {
        visit_UnaryOp(x);
    }
    void visit_RealUnaryMinus(const ASR::RealUnaryMinus_t &x) {
        visit_UnaryOp(x);
    }
    void visit_ComplexUnaryMinus(const ASR::ComplexUnaryMinus_t &x) {
        visit_UnaryOp(x);
    }
    void visit_IntegerBitNot(const ASR::IntegerBitNot_t &x) {
        visit_UnaryOp(x);
    }
    void visit_LogicalNot(const ASR::LogicalNot_t &x) {
        visit_UnaryOp(x);
    }

    template<typename T>
    void visit_UnaryOp(const T& x) {
        from_fma = true;
        T& xx = const_cast<T&>(x);
        fma_var = nullptr;
        visit_expr(*x.m_arg);
        if( fma_var ) {
            xx.m_arg = fma_var;
        }
        fma_var = nullptr;
        from_fma = false;
    }

};

void pass_replace_fma(Allocator &al, ASR::TranslationUnit_t &unit,
                      const LCompilers::PassOptions& pass_options) {
    FMAVisitor v(al, unit, pass_options);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
