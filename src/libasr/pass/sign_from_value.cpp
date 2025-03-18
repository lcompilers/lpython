#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/replace_sign_from_value.h>
#include <libasr/pass/pass_utils.h>

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

class SignFromValueReplacer : public ASR::BaseExprReplacer<SignFromValueReplacer>{
private:
    Allocator &al;
    ASR::TranslationUnit_t &unit;
    const LCompilers::PassOptions& pass_options;

    bool is_value_one(ASR::expr_t* expr) {
        double value;
        if( ASRUtils::is_value_constant(ASRUtils::expr_value(expr), value) && 
            ASRUtils::is_real(*ASRUtils::expr_type(expr)) ) {
            return value == 1.0;
        }
        return false;
    }

    ASR::expr_t* is_extract_sign(ASR::expr_t* expr) {
        if( ASR::is_a<ASR::RealCopySign_t>(*expr) ) {
            ASR::RealCopySign_t *real_cpy_sign = ASR::down_cast<ASR::RealCopySign_t>(expr);
            if( !is_value_one(real_cpy_sign->m_target) ) {return nullptr;}
            return real_cpy_sign->m_source;
        }
        return nullptr;
    }


public:

    SignFromValueReplacer(Allocator &al, ASR::TranslationUnit_t &unit,
                                  const LCompilers::PassOptions& pass_options) 
                            :al(al),unit(unit), pass_options(pass_options){}


    void replace_RealBinOp(ASR::RealBinOp_t* x) {
        BaseExprReplacer::replace_RealBinOp(x);

        if( x->m_op != ASR::binopType::Mul ) { return; }
        ASR::expr_t *first_arg = nullptr, *second_arg = nullptr;
        first_arg = is_extract_sign(x->m_left);
        second_arg = is_extract_sign(x->m_right);

        if( second_arg ) {
            first_arg = x->m_left;
        } else if( first_arg ) {
            second_arg = x->m_right;
        } else {
            return ;
        }
        *current_expr = PassUtils::get_sign_from_value(first_arg, second_arg,
                                     al, unit, x->base.base.loc,
                                    const_cast<LCompilers::PassOptions&>(pass_options));
    }
    
};

class SignFromValueVisitor : public ASR::CallReplacerOnExpressionsVisitor<SignFromValueVisitor>{
private:
    SignFromValueReplacer replacer;
public:
    SignFromValueVisitor(Allocator &al, ASR::TranslationUnit_t &unit,
                                  const LCompilers::PassOptions& pass_options)
                        :replacer{al, unit, pass_options}{}
                        
    void call_replacer(){
        if( is_a<ASR::RealBinOp_t>(**current_expr) ){
            replacer.current_expr = current_expr;
            replacer.replace_expr(*current_expr);
        }
    }
    void visit_Function(const ASR::Function_t &x){
        if(std::string(x.m_name).find("_lcompilers_optimization_")
            !=std::string::npos){ // Don't visit the optimization functions.
            return;
        }
    }
};
void pass_replace_sign_from_value(Allocator &al, ASR::TranslationUnit_t &unit,
                                  const LCompilers::PassOptions& pass_options) {
    SignFromValueVisitor sign_from_value_visitor(al, unit, pass_options);
    sign_from_value_visitor.visit_TranslationUnit(unit);
    
}


} // namespace LCompilers
