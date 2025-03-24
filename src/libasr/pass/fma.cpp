#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_fma.h>
#include <libasr/pass/pass_utils.h>


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

class ReplaceRealBinOpFMA : public ASR::BaseExprReplacer<ReplaceRealBinOpFMA>{
    Allocator& al;
    const LCompilers::PassOptions& pass_options;
    const ASR::TranslationUnit_t& unit;

    bool is_BinOpMul(ASR::expr_t* expr) {
        if (ASR::is_a<ASR::RealBinOp_t>(*expr)) {
            ASR::RealBinOp_t* expr_binop = ASR::down_cast<ASR::RealBinOp_t>(expr);
            return expr_binop->m_op == ASR::binopType::Mul;
        }
        return false;
    }

public :
    ReplaceRealBinOpFMA(Allocator& al, 
        const LCompilers::PassOptions& pass_options, const ASR::TranslationUnit_t& unit)
        :al{al}, pass_options{pass_options}, unit{unit}{}
    
    void replace_RealBinOp(ASR::RealBinOp_t* x) {
        BaseExprReplacer::replace_RealBinOp(x);

        if( x->m_op != ASR::binopType::Add && x->m_op != ASR::binopType::Sub ) {
            return ;
        }
        ASR::expr_t *mul_expr = nullptr, *other_expr = nullptr;
        bool is_mul_expr_negative = false, is_other_expr_negative = false;
        if( is_BinOpMul(x->m_right) ) {
            mul_expr = x->m_right;
            other_expr = x->m_left;
            is_mul_expr_negative = (x->m_op == ASR::binopType::Sub);
        } else if( is_BinOpMul(x->m_left) ) {
            mul_expr = x->m_left;
            other_expr = x->m_right;
            is_other_expr_negative = (x->m_op == ASR::binopType::Sub);
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

        *current_expr = PassUtils::get_fma(other_expr, first_arg, second_arg,
                                     al, const_cast<ASR::TranslationUnit_t&>(unit), x->base.base.loc, 
                                     const_cast<LCompilers::PassOptions&>(pass_options));
    }


};



class CallReplacerFMA : public ASR::CallReplacerOnExpressionsVisitor<CallReplacerFMA>{
    ReplaceRealBinOpFMA replacer;
public :
    CallReplacerFMA(Allocator &al, const ASR::TranslationUnit_t &unit,
                      const LCompilers::PassOptions& pass_options) 
                      : replacer{al, pass_options, unit}{}
    void call_replacer(){
        if(ASR::is_a<ASR::RealBinOp_t>(**current_expr)){
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }
    }
    void visit_Function(const ASR::Function_t& x){ // Avoid visiting RealBinOp in the fma` function itself
        if(std::string(x.m_name) == "_lcompilers_optimization_fma_f32" || 
            std::string(x.m_name) == "_lcompilers_optimization_fma_f64"){
            return;
        }
        CallReplacerOnExpressionsVisitor::visit_Function(x);
    }
    
};


void pass_replace_fma(Allocator &al, ASR::TranslationUnit_t &unit,
                      const LCompilers::PassOptions& pass_options) {
    CallReplacerFMA realBinOpFMA_replacer{al, unit, pass_options};
    realBinOpFMA_replacer.visit_TranslationUnit(unit);
    PassUtils::UpdateDependenciesVisitor u(al);
    u.visit_TranslationUnit(unit);
}


} // namespace LCompilers
