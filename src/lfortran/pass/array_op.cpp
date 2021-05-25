#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/array_op.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*
This ASR pass replaces operations over arrays with do loops. 
The function `pass_replace_array_op` transforms the ASR tree in-place.

Converts:

    c = a + b

to:

    do i = lbound(a), ubound(a)
        c(i) = a(i) + b(i)
    end do
*/

class ArrayOpVisitor : public ASR::BaseWalkVisitor<ArrayOpVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> array_op_result;
    bool apply_pass;

public:
    ArrayOpVisitor(Allocator &al) : al{al}, apply_pass{false} {
        array_op_result.reserve(al, 1);

    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            array_op_result.n = 0;
            visit_stmt(*m_body[i]);
            if (array_op_result.size() > 0) {
                for (size_t j=0; j<array_op_result.size(); j++) {
                    body.push_back(al, array_op_result[j]);
                }
                array_op_result.n = 0;
            } else {
                body.push_back(al, m_body[i]);
            }
        }
        m_body = body.p;
        n_body = body.size();
    }

    // TODO: Only Program and While is processed, we need to process all calls
    // to visit_stmt().

    void visit_Program(const ASR::Program_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Program_t &xx = const_cast<ASR::Program_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);

        // Transform nested functions and subroutines
        for (auto &item : x.m_symtab->scope) {
            if (is_a<ASR::Subroutine_t>(*item.second)) {
                ASR::Subroutine_t *s = down_cast<ASR::Subroutine_t>(item.second);
                visit_Subroutine(*s);
            }
            if (is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = down_cast<ASR::Function_t>(item.second);
                visit_Function(*s);
            }
        }
    }

    void visit_Subroutine(const ASR::Subroutine_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Subroutine_t &xx = const_cast<ASR::Subroutine_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_Function(const ASR::Function_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::Function_t &xx = const_cast<ASR::Function_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    inline bool is_array(ASR::expr_t* x) {
        bool result = false;
        int n_dims = 0;
        ASR::ttype_t* x_type = expr_type(x);
        if( x->type == ASR::exprType::Var ) {
            switch( x->type ) {
                case ASR::ttypeType::Integer: {
                    ASR::Integer_t* x_type_ref = down_cast<ASR::Integer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::IntegerPointer: {
                    ASR::IntegerPointer_t* x_type_ref = down_cast<ASR::IntegerPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Real: {
                    ASR::Real_t* x_type_ref = down_cast<ASR::Real_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::RealPointer: {
                    ASR::RealPointer_t* x_type_ref = down_cast<ASR::RealPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Complex: {
                    ASR::Complex_t* x_type_ref = down_cast<ASR::Complex_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::ComplexPointer: {
                    ASR::ComplexPointer_t* x_type_ref = down_cast<ASR::ComplexPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Derived: {
                    ASR::Derived_t* x_type_ref = down_cast<ASR::Derived_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::DerivedPointer: {
                    ASR::DerivedPointer_t* x_type_ref = down_cast<ASR::DerivedPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Logical: {
                    ASR::Logical_t* x_type_ref = down_cast<ASR::Logical_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::LogicalPointer: {
                    ASR::LogicalPointer_t* x_type_ref = down_cast<ASR::LogicalPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::Character: {
                    ASR::Character_t* x_type_ref = down_cast<ASR::Character_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                case ASR::ttypeType::CharacterPointer: {
                    ASR::CharacterPointer_t* x_type_ref = down_cast<ASR::CharacterPointer_t>(x_type);
                    n_dims = x_type_ref->n_dims;
                    break;
                }
                default:
                    break;
            }
            result = n_dims > 0;
        }
        std::cout<<result<<std::endl;
        return result;
    }

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( is_array(x.m_target) ) {
            apply_pass = true;
            this->visit_expr(*(x.m_value));
            apply_pass = false;
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        std::cout<<"Inside BinOp"<<std::endl;
    }
};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayOpVisitor v(al);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
