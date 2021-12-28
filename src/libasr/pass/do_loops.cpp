#include <libasr/asr.h>
#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/pass/do_loops.h>
#include <libasr/pass/stmt_walk_visitor.h>

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
        throw LFortranException("Do loop increment type not supported");
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
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, a, ASR::binopType::Sub, c, type, nullptr, nullptr)),
        nullptr));

    ASR::expr_t *cond = LFortran::ASRUtils::EXPR(ASR::make_Compare_t(al, loc,
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr, nullptr)),
        cmp_op, b, type, nullptr, nullptr));
    Vec<ASR::stmt_t*> body;
    body.reserve(al, loop.n_body+1);
    body.push_back(al, LFortran::ASRUtils::STMT(ASR::make_Assignment_t(al, loc, target,
        LFortran::ASRUtils::EXPR(ASR::make_BinOp_t(al, loc, target, ASR::binopType::Add, c, type, nullptr, nullptr)),
    nullptr)));
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

class DoLoopVisitor : public ASR::StatementWalkVisitor<DoLoopVisitor>
{
public:
    DoLoopVisitor(Allocator &al) : StatementWalkVisitor(al) {
    }

    void visit_DoLoop(const ASR::DoLoop_t &x) {
        stmts = replace_doloop(al, x);
    }
};

void pass_replace_do_loops(Allocator &al, ASR::TranslationUnit_t &unit) {
    DoLoopVisitor v(al);
    // Each call transforms only one layer of nested loops, so we call it twice
    // to transform doubly nested loops:
    v.asr_changed = true;
    while( v.asr_changed ) {
        v.asr_changed = false;
        v.visit_TranslationUnit(unit);
    }
    LFORTRAN_ASSERT(asr_verify(unit));
}


} // namespace LFortran
