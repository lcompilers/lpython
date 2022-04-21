#include <libasr/asr.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/param_to_const.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces initializer expressions with evaluated values. The function
`pass_replace_param_to_const` transforms the ASR tree in-place.

Converts:

    integer, parameter :: i = 20
    integer :: a = i**2 + i

to:

    integer, parameter :: i = 20
    integer :: a = 20**2 + 20

*/

class VarVisitor : public ASR::BaseWalkVisitor<VarVisitor>
{
private:
    ASR::expr_t* asr;

public:
    // Currently `al` is not used, but we might need it in the future:
    Allocator &al;

    VarVisitor(Allocator &al) : asr{nullptr}, al{al} {
    }

    void visit_Program(const ASR::Program_t &x) {
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Variable_t>(*item.second)) {
                ASR::Variable_t *v = down_cast<ASR::Variable_t>(item.second);
                visit_Variable(*v);
            }
        }
    }

    void visit_UnaryOp(const ASR::UnaryOp_t& x) {
        ASR::UnaryOp_t& x_unconst = const_cast<ASR::UnaryOp_t&>(x);
        asr = nullptr;
        this->visit_expr(*x.m_operand);
        if( asr != nullptr ) {
            x_unconst.m_operand = asr;
        }
        asr = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_StrOp(const ASR::StrOp_t& x) {
        ASR::StrOp_t& x_unconst = const_cast<ASR::StrOp_t&>(x);
        asr = nullptr;
        this->visit_expr(*x.m_left);
        if( asr != nullptr ) {
            x_unconst.m_left = asr;
        }
        asr = nullptr;
        this->visit_expr(*x.m_right);
        if( asr != nullptr ) {
            x_unconst.m_right = asr;
        }
        asr = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_Compare(const ASR::Compare_t& x) {
        ASR::Compare_t& x_unconst = const_cast<ASR::Compare_t&>(x);
        asr = nullptr;
        this->visit_expr(*x.m_left);
        if( asr != nullptr ) {
            x_unconst.m_left = asr;
        }
        asr = nullptr;
        this->visit_expr(*x.m_right);
        if( asr != nullptr ) {
            x_unconst.m_right = asr;
        }
        asr = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_BinOp(const ASR::BinOp_t& x) {
        ASR::BinOp_t& x_unconst = const_cast<ASR::BinOp_t&>(x);
        asr = nullptr;
        this->visit_expr(*x.m_left);
        if( asr != nullptr ) {
            x_unconst.m_left = asr;
        }
        asr = nullptr;
        this->visit_expr(*x.m_right);
        if( asr != nullptr ) {
            x_unconst.m_right = asr;
        }
        asr = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_Cast(const ASR::Cast_t& x) {
        /*
        asr = nullptr;
        this->visit_expr(*x.m_arg);
        ASR::Cast_t& x_unconst = const_cast<ASR::Cast_t&>(x);
        if( asr != nullptr ) {
            x_unconst.m_arg = asr;
        }
        */
        asr = const_cast<ASR::expr_t*>(&(x.base));
    }

    void visit_Var(const ASR::Var_t& x) {
        if (is_a<ASR::Variable_t>(*LFortran::ASRUtils::symbol_get_past_external(x.m_v))) {
            ASR::Variable_t *init_var = ASR::down_cast<ASR::Variable_t>(LFortran::ASRUtils::symbol_get_past_external(x.m_v));
            if( init_var->m_storage == ASR::storage_typeType::Parameter ) {
                if( init_var->m_symbolic_value == nullptr ) {
                    asr = init_var->m_symbolic_value;
                } else {
                    switch( init_var->m_symbolic_value->type ) {
                        case ASR::exprType::IntegerConstant:
                        case ASR::exprType::RealConstant:
                        case ASR::exprType::ComplexConstant:
                        case ASR::exprType::LogicalConstant:
                        case ASR::exprType::StringConstant: {
                            asr = init_var->m_symbolic_value;
                            break;
                        }
                        default: {
                            this->visit_expr(*(init_var->m_symbolic_value));
                        }
                    }
                }
            }
        }
    }

    void visit_Variable(const ASR::Variable_t& x) {
        ASR::Variable_t& x_unconst = const_cast<ASR::Variable_t&>(x);
        if( x.m_symbolic_value != nullptr ) {
            asr = nullptr;
            visit_expr(*(x.m_symbolic_value));
            if( asr != nullptr ) {
                x_unconst.m_symbolic_value = asr;
                asr = nullptr;
            }
        }
    }
};

void pass_replace_param_to_const(Allocator &al, ASR::TranslationUnit_t &unit) {
    VarVisitor v(al);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}

} // namespace LFortran
