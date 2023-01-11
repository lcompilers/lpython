#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/div_to_mul.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces divison operation with multiplication
if the divisor can be evaluated to a constant at compile time.

Converts:

    real :: x
    real, parameter :: divisor = 2.0
    print *, x/divisor

to:

    real :: x
    real, parameter :: divisor = 2.0
    print *, x * 0.5

*/
class DivToMulVisitor : public PassUtils::PassVisitor<DivToMulVisitor>
{
private:

    std::string rl_path;

public:
    DivToMulVisitor(Allocator &al_, const std::string& rl_path_) : PassVisitor(al_, nullptr),
    rl_path(rl_path_)
    {
        pass_result.reserve(al, 1);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        visit_expr(*x.m_left);
        visit_expr(*x.m_right);
        if( x.m_op == ASR::binopType::Div ) {
            ASR::expr_t* right_value = ASRUtils::expr_value(x.m_right);
            if( right_value ) {
                double value;
                if( ASRUtils::extract_value<double>(right_value, value) ) {
                    bool is_feasible = false;
                    ASR::expr_t* right_inverse = nullptr;
                    switch( x.m_type->type ) {
                        case ASR::ttypeType::Real: {
                            is_feasible = true;
                            right_inverse = ASRUtils::EXPR(ASR::make_RealConstant_t(al, x.m_right->base.loc, 1.0/value, x.m_type));
                            break;
                        }
                        default:
                            break;
                    }
                    if( is_feasible ) {
                        ASR::RealBinOp_t& xx = const_cast<ASR::RealBinOp_t&>(x);
                        xx.m_op = ASR::binopType::Mul;
                        xx.m_right = right_inverse;
                    }
                }
            }
        }
    }

};

void pass_replace_div_to_mul(Allocator &al, ASR::TranslationUnit_t &unit,
                             const LCompilers::PassOptions& pass_options) {
    std::string rl_path = pass_options.runtime_library_dir;
    DivToMulVisitor v(al, rl_path);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
