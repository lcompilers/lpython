#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/pass/do_loops.h>


namespace LFortran {

using ASR::down_cast;
using ASR::is_a;

/*

This ASR pass replaces do loops with while loops. The function
`pass_replace_do_loops` transforms the ASR tree in-place.

Converts:

    do i = a, b, c
        ...
    end do

to:

    i = a-c
    do while (i+c <= b)
        i = i+c
        ...
    end do

The comparison is >= for c<0.
*/
Vec<ASR::stmt_t*> replace_doloop(Allocator &al, const ASR::DoLoop_t &loop) {
    Location loc = loop.base.base.loc;
    ASR::expr_t *a=loop.m_head.m_start;
    ASR::expr_t *b=loop.m_head.m_end;
    ASR::expr_t *c=loop.m_head.m_increment;
    LFORTRAN_ASSERT(a);
    LFORTRAN_ASSERT(b);
    if (!c) {
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
        c = LFortran::ASRUtils::EXPR(ASR::make_ConstantInteger_t(al, loc, 1, type));
    }
    LFORTRAN_ASSERT(c);
    int increment;
    if (c->type == ASR::exprType::ConstantInteger) {
        increment = down_cast<ASR::ConstantInteger_t>(c)->m_n;
    } else if (c->type == ASR::exprType::UnaryOp) {
        ASR::UnaryOp_t *u = down_cast<ASR::UnaryOp_t>(c);
        LFORTRAN_ASSERT(u->m_op == ASR::unaryopType::USub);
        LFORTRAN_ASSERT(u->m_operand->type == ASR::exprType::ConstantInteger);
        increment = - down_cast<ASR::ConstantInteger_t>(u->m_operand)->m_n;
    } else {
        throw CodeGenError("Do loop increment type not supported");
    }
    ASR::cmpopType cmp_op;
    if (increment > 0) {
        cmp_op = ASR::cmpopType::LtE;
    } else {
        cmp_op = ASR::cmpopType::GtE;
    }
    ASR::expr_t *target = loop.m_head.m_v;
    ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
    ASR::stmt_t *stmt1 = LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, a, ASR::binopType::Sub, c, type, nullptr))
    ));

    ASR::expr_t *cond = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc,
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr)),
        cmp_op, b, type, nullptr));
    Vec<ASR::stmt_t*> body;
    body.reserve(al, loop.n_body+1);
    body.push_back(al, LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr))
    )));
    for (size_t i=0; i<loop.n_body; i++) {
        body.push_back(al, loop.m_body[i]);
    }
    ASR::stmt_t *stmt2 = LFortran::ASRUtils::STMT(ASR::make_WhileLoop_t(al, loc, cond,
        body.p, body.size()));
    Vec<ASR::stmt_t*> result;
    result.reserve(al, 2);
    result.push_back(al, stmt1);
    result.push_back(al, stmt2);

    /*
    std::cout << "Input:" << std::endl;
    std::cout << pickle((ASR::asr_t&)loop);
    std::cout << "Output:" << std::endl;
    std::cout << pickle((ASR::asr_t&)*stmt1);
    std::cout << pickle((ASR::asr_t&)*stmt2);
    std::cout << "--------------" << std::endl;
    */

    return result;
}

class DoLoopVisitor : public ASR::BaseWalkVisitor<DoLoopVisitor>
{
private:
    Allocator &al;
    Vec<ASR::stmt_t*> do_loop_result;

public:
    bool is_do_loop_present;

    DoLoopVisitor(Allocator &al) : al{al}, is_do_loop_present{false} {
        do_loop_result.n = 0;

    }

    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, n_body);
        for (size_t i=0; i<n_body; i++) {
            // Not necessary after we check it after each visit_stmt in every
            // visitor method:
            do_loop_result.n = 0;
            visit_stmt(*m_body[i]);
            if (do_loop_result.size() > 0) {
                is_do_loop_present = true;
                for (size_t j=0; j<do_loop_result.size(); j++) {
                    body.push_back(al, do_loop_result[j]);
                }
                do_loop_result.n = 0;
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

    void visit_WhileLoop(const ASR::WhileLoop_t &x) {
        // FIXME: this is a hack, we need to pass in a non-const `x`,
        // which requires to generate a TransformVisitor.
        ASR::WhileLoop_t &xx = const_cast<ASR::WhileLoop_t&>(x);
        transform_stmts(xx.m_body, xx.n_body);
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        do_loop_result = replace_doloop(al, x);
    }
};

void pass_replace_do_loops(Allocator &al, ASR::TranslationUnit_t &unit) {
    DoLoopVisitor v(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.is_do_loop_present = true;
    while( v.is_do_loop_present ) {
        v.is_do_loop_present = false;
        v.visit_TranslationUnit(unit);
    }
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
