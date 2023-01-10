#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/flip_sign.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <utility>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR replaces flip sign operation with bit shifts for enhancing efficiency.

Converts:

    if (modulo(number, 2) == 1 ) x = -x

to:

    x = xor(shiftl(int(number), 31), x)

For 64 bit `number`, 31 is replaced with 63.

The algorithm contains two components,

1. Detecting Flip Signs - This is achieved by looking for the existence
    of a specific subtree in the complete ASR tree. In this case, we are
    looking for a subtree which has an `If` node as the parent node. The
    `Compare` attribute of that `If` node should contain a `FunctionCall`
    to `modulo` (with second argument as `2`) and a `IntegerConstant`, `1`.
    The statement attribute of `If` node should contain only 1 and that too
    an `Assignment` statement. The right should be a `UnaryOp` expression
    with operand as the left hand side of that expression.

    For achieving this `FlipSignVisitor` has attributes which are
    set to True only when the above properties are satisfied in an
    order. For example, `is_function_call_present` will be set to `True`
    only when we have visited `Compare` which means `is_compare_present`
    is already `True` at that point.

2. Replacing Flip Sign with bit shifts - This phase is executed only
    when the above one is a success. Here, we replace the subtree
    detected above with a call to a generic procedure defined in
    `lfortran_intrinsic_optimization`. This is just a dummy call
    anyways to keep things backend agnostic. The actual implementation
    will be generated in the backend specified.


*/
class FlipSignVisitor : public PassUtils::SkipOptimizationFunctionVisitor<FlipSignVisitor>
{
private:
    ASR::TranslationUnit_t &unit;

    LCompilers::PassOptions pass_options;

    ASR::expr_t *flip_sign_signal_variable, *flip_sign_variable;

    bool is_if_present;
    bool is_compare_present;
    bool is_function_call_present, is_function_modulo, is_divisor_2;
    bool is_one_present;
    bool is_unary_op_present, is_operand_same_as_input;
    bool is_flip_sign_present;

public:
    FlipSignVisitor(Allocator &al_, ASR::TranslationUnit_t &unit_,
                    const LCompilers::PassOptions& pass_options_) : SkipOptimizationFunctionVisitor(al_),
    unit(unit_), pass_options(pass_options_)
    {
        pass_result.reserve(al, 1);
    }

    void visit_If(const ASR::If_t& x) {
        is_if_present = true;
        is_compare_present = false;
        is_function_call_present = false;
        is_function_modulo = false, is_divisor_2 = false;
        is_one_present = false;
        is_unary_op_present = false;
        is_operand_same_as_input = false;
        flip_sign_signal_variable = flip_sign_variable = nullptr;
        visit_expr(*(x.m_test));
        if( x.n_body == 1 && x.n_orelse == 0 ) {
            if( x.m_body[0]->type == ASR::stmtType::Assignment ) {
                visit_stmt(*(x.m_body[0]));
            }
        }
        set_flip_sign();
        if( is_flip_sign_present ) {
            // xi = xor(shiftl(int(Nd),63), xi)
            LCOMPILERS_ASSERT(flip_sign_signal_variable);
            LCOMPILERS_ASSERT(flip_sign_variable);
            ASR::stmt_t* flip_sign_call = PassUtils::get_flipsign(flip_sign_signal_variable,
                                            flip_sign_variable, al, unit, pass_options, current_scope,
                                            [&](const std::string &msg, const Location &) { throw LCompilersException(msg); });
            pass_result.push_back(al, flip_sign_call);
        }
    }

    void set_flip_sign() {
        is_flip_sign_present = (is_if_present &&
                                is_compare_present &&
                                is_function_call_present &&
                                is_function_modulo &&
                                is_divisor_2 &&
                                is_one_present &&
                                is_unary_op_present &&
                                is_operand_same_as_input);
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( x.m_value->type == ASR::exprType::RealUnaryMinus ) {
            is_unary_op_present = true;
            ASR::symbol_t* sym = nullptr;
            ASR::RealUnaryMinus_t* negation = ASR::down_cast<ASR::RealUnaryMinus_t>(x.m_value);
            if( negation->m_arg->type == ASR::exprType::Var ) {
                ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(negation->m_arg);
                sym = var->m_v;
            }
            if( x.m_target->type == ASR::exprType::Var ) {
                ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(x.m_target);
                is_operand_same_as_input = sym == var->m_v;
                flip_sign_variable = x.m_target;
            }
        }
    }

    void visit_IntegerCompare(const ASR::IntegerCompare_t& x) {
        handle_Compare(x);
    }

    void visit_RealCompare(const ASR::RealCompare_t &x) {
        handle_Compare(x);
    }

    void visit_ComplexCompare(const ASR::ComplexCompare_t &x) {
        handle_Compare(x);
    }

    void visit_LogicalCompare(const ASR::LogicalCompare_t &x) {
        handle_Compare(x);
    }

    void visit_StringCompare(const ASR::StringCompare_t &x) {
        handle_Compare(x);
    }

    template <typename T>
    void handle_Compare(const T& x) {
        is_compare_present = true;
        ASR::expr_t* potential_one = nullptr;
        ASR::expr_t* potential_func_call = nullptr;
        if( x.m_left->type == ASR::exprType::FunctionCall ) {
            potential_one = x.m_right;
            potential_func_call = x.m_left;
        } else if( x.m_right->type == ASR::exprType::FunctionCall ) {
            potential_one = x.m_left;
            potential_func_call = x.m_right;
        }
        if( potential_one &&
            potential_one->type == ASR::exprType::IntegerConstant ) {
            ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(potential_one);
            is_one_present = const_int->m_n == 1;
        }
        if( potential_func_call && is_one_present ) {
            visit_expr(*potential_func_call);
        }
    }

    void visit_FunctionCall(const ASR::FunctionCall_t& x) {
        is_function_call_present = true;
        ASR::symbol_t* func_name = nullptr;
        if( x.m_original_name ) {
            func_name = x.m_original_name;
        } else if( x.m_name ) {
            func_name = x.m_name;
        }
        if( func_name && func_name->type == ASR::symbolType::ExternalSymbol ) {
            ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(func_name);
            if( std::string(ext_sym->m_original_name) == "modulo" &&
                std::string(ext_sym->m_module_name) == "lfortran_intrinsic_math2" ) {
                is_function_modulo = true;
            }
        }
        if( is_function_modulo && x.n_args == 2) {
            ASR::expr_t* arg0 = x.m_args[0].m_value;
            ASR::expr_t* arg1 = x.m_args[1].m_value;
            bool cond_for_arg0 = false, cond_for_arg1 = false;
            ASR::ttype_t* arg0_ttype = ASRUtils::expr_type(arg0);
            cond_for_arg0 = arg0_ttype->type == ASR::ttypeType::Integer;
            if( arg1->type == ASR::exprType::IntegerConstant ) {
                ASR::IntegerConstant_t* const_int = ASR::down_cast<ASR::IntegerConstant_t>(arg1);
                cond_for_arg1 = const_int->m_n == 2;
            }
            is_divisor_2 = cond_for_arg0 && cond_for_arg1;
            flip_sign_signal_variable = arg0;
        }
    }

};

void pass_replace_flip_sign(Allocator &al, ASR::TranslationUnit_t &unit,
                            const LCompilers::PassOptions& pass_options) {
    FlipSignVisitor v(al, unit, pass_options);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
