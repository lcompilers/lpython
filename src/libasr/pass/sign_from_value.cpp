#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_sign_from_value.h>
#include <libasr/pass/pass_utils.h>

#include <vector>
#include <string>


namespace LCompilers {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass converts assigning of sign from a value to
different value with a call to internal optimization routine.
This allows backend specific code generation for better performance.

Converts:

    real :: a, b, c
    c = a*sign(1.0, b)

to:

    c = sign_from_value(a, b)

*/
class SignFromValueVisitor : public PassUtils::SkipOptimizationFunctionVisitor<SignFromValueVisitor>
{
private:
    ASR::TranslationUnit_t &unit;

    LCompilers::PassOptions pass_options;

    ASR::expr_t* sign_from_value_var;

    // To make sure that SignFromValue is applied only for
    // the nodes implemented in this class
    bool from_sign_from_value;

public:
    SignFromValueVisitor(Allocator &al_, ASR::TranslationUnit_t &unit_,
                         const LCompilers::PassOptions& pass_options_) : SkipOptimizationFunctionVisitor(al_),
    unit(unit_), pass_options(pass_options_), sign_from_value_var(nullptr), from_sign_from_value(false)
    {
        pass_result.reserve(al, 1);
    }

    bool is_value_one(ASR::expr_t* expr) {
        double value;
        if( ASRUtils::is_value_constant(expr, value) ) {
            return value == 1.0;
        }
        return false;
    }

    ASR::expr_t* is_extract_sign(ASR::expr_t* expr) {
        if( !ASR::is_a<ASR::FunctionCall_t>(*expr) ) {
            return nullptr;
        }
        ASR::FunctionCall_t* func_call = ASR::down_cast<ASR::FunctionCall_t>(expr);
        ASR::symbol_t* func_sym = ASRUtils::symbol_get_past_external(func_call->m_name);
        if( !ASR::is_a<ASR::Function_t>(*func_sym) ) {
            return nullptr;
        }
        ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(func_sym);
        if( ASRUtils::is_intrinsic_procedure(func) &&
            std::string(func->m_name).find("sign") == std::string::npos ) {
            return nullptr;
        }
        ASR::expr_t *arg0 = func_call->m_args[0].m_value, *arg1 = func_call->m_args[1].m_value;
        if( !is_value_one(arg0) ) {
            return nullptr;
        }
        return arg1;
    }

    void visit_IntegerBinOp(const ASR::IntegerBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_RealBinOp(const ASR::RealBinOp_t& x) {
        handle_BinOp(x);
    }

    void visit_ComplexBinOp(const ASR::ComplexBinOp_t& x) {
        handle_BinOp(x);
    }

    template <typename T>
    void handle_BinOp(const T& x_const) {
        if( !from_sign_from_value ) {
            return ;
        }

        from_sign_from_value = true;
        T& x = const_cast<T&>(x_const);

        sign_from_value_var = nullptr;
        visit_expr(*x.m_left);
        if( sign_from_value_var ) {
            x.m_left = sign_from_value_var;
        }

        sign_from_value_var = nullptr;
        visit_expr(*x.m_right);
        if( sign_from_value_var ) {
            x.m_right = sign_from_value_var;
        }
        sign_from_value_var = nullptr;

        if( x.m_op != ASR::binopType::Mul ) {
            return ;
        }

        ASR::expr_t *first_arg = nullptr, *second_arg = nullptr;

        first_arg = is_extract_sign(x.m_left);
        second_arg = is_extract_sign(x.m_right);

        if( second_arg ) {
            first_arg = x.m_left;
        } else if( first_arg ) {
            second_arg = x.m_right;
        } else {
            return ;
        }

        sign_from_value_var = PassUtils::get_sign_from_value(first_arg, second_arg,
                                     al, unit, pass_options, current_scope, x.base.base.loc,
                                     [&](const std::string &msg, const Location &) { throw LCompilersException(msg); });
        from_sign_from_value = false;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        from_sign_from_value = true;
        ASR::Assignment_t& xx = const_cast<ASR::Assignment_t&>(x);
        sign_from_value_var = nullptr;
        visit_expr(*x.m_value);
        if( sign_from_value_var ) {
            xx.m_value = sign_from_value_var;
        }
        sign_from_value_var = nullptr;
        from_sign_from_value = false;
    }

};

void pass_replace_sign_from_value(Allocator &al, ASR::TranslationUnit_t &unit,
                                  const LCompilers::PassOptions& pass_options) {
    SignFromValueVisitor v(al, unit, pass_options);
    v.visit_TranslationUnit(unit);
}


} // namespace LCompilers
