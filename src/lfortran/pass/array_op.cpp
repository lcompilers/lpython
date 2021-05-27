#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/array_op.h>
#include <lfortran/pass/pass_utils.h>


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
    ASR::TranslationUnit_t &unit;
    Vec<ASR::stmt_t*> array_op_result;
    bool apply_pass;
    ASR::expr_t* m_target;

public:
    ArrayOpVisitor(Allocator &al, ASR::TranslationUnit_t &unit) : al{al}, unit{unit}, apply_pass{false}, m_target{nullptr} {
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

    void visit_Assignment(const ASR::Assignment_t& x) {
        if( PassUtils::is_array(x.m_target, al) ) {
            apply_pass = true;
            m_target = x.m_target;
            this->visit_expr(*(x.m_value));
            m_target = nullptr;
            apply_pass = false;
        }
    }

    void visit_BinOp(const ASR::BinOp_t &x) {
        if( !apply_pass ) {
            return ;
        }
        // Case #1: For static arrays in both the operands
        Vec<PassUtils::dimension_descriptor> dims_left_vec;
        PassUtils::get_dims(x.m_left, dims_left_vec, al);
        Vec<PassUtils::dimension_descriptor> dims_right_vec;
        PassUtils::get_dims(x.m_right, dims_right_vec, al);
        if( dims_left_vec.size() != dims_right_vec.size() ) {
            throw SemanticError("Cannot generate loop operands of different shapes", 
                                x.base.base.loc);
        }
        if( dims_left_vec.size() <= 0 ) {
            return ;
        }

        PassUtils::dimension_descriptor* dims_left = dims_left_vec.p;
        int n_dims = dims_left_vec.size();
        PassUtils::dimension_descriptor* dims_right = dims_right_vec.p;
        ASR::ttype_t* int32_type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
        Vec<ASR::expr_t*> idx_vars;
        PassUtils::create_idx_vars(idx_vars, n_dims, x.base.base.loc, al, unit);
        ASR::stmt_t* doloop = nullptr;
        for( int i = n_dims - 1; i >= 0; i-- ) {
            if( dims_left[i].lbound != dims_right[i].lbound ||
                dims_left[i].ubound != dims_right[i].ubound ) {
                throw SemanticError("Incompatible shapes of operand arrays", x.base.base.loc);
            }
            ASR::do_loop_head_t head;
            head.m_v = idx_vars[i];
            head.m_start = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims_left[i].lbound, int32_type)); // TODO: Replace with call to lbound
            head.m_end = EXPR(ASR::make_ConstantInteger_t(al, x.base.base.loc, dims_left[i].ubound, int32_type)); // TODO: Replace with call to ubound
            head.m_increment = nullptr;
            head.loc = head.m_v->base.loc;
            Vec<ASR::stmt_t*> doloop_body;
            doloop_body.reserve(al, 1);
            if( doloop == nullptr ) {
                ASR::expr_t* ref_1 = PassUtils::create_array_ref(x.m_left, idx_vars, al);
                ASR::expr_t* ref_2 = PassUtils::create_array_ref(x.m_right, idx_vars, al);
                ASR::expr_t* res = PassUtils::create_array_ref(m_target, idx_vars, al);
                ASR::expr_t* bin_op_el_wise = EXPR(ASR::make_BinOp_t(al, x.base.base.loc, ref_1, x.m_op, ref_2, x.m_type));
                ASR::stmt_t* assign = STMT(ASR::make_Assignment_t(al, x.base.base.loc, res, bin_op_el_wise));
                doloop_body.push_back(al, assign);
            } else {
                doloop_body.push_back(al, doloop);
            }
            doloop = STMT(ASR::make_DoLoop_t(al, x.base.base.loc, head, doloop_body.p, doloop_body.size()));
        }
        array_op_result.push_back(al, doloop);
    }
};

void pass_replace_array_op(Allocator &al, ASR::TranslationUnit_t &unit) {
    ArrayOpVisitor v(al, unit);
    v.visit_TranslationUnit(unit);
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
