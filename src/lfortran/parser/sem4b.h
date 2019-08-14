#ifndef LFORTRAN_PARSER_SEM4b_H
#define LFORTRAN_PARSER_SEM4b_H

#include <lfortran/casts.h>
#include <lfortran/parser/alloc.h>
#include <lfortran/ast.h>

namespace LFortran {

extern Allocator al;

}

using LFortran::al;

using LFortran::AST::exprType;
using LFortran::AST::operatorType;
using LFortran::AST::expr_t;
using LFortran::AST::BinOp_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;
typedef struct LFortran::AST::expr_t *PExpr;



static PExpr make_binop(operatorType type, PExpr x, PExpr y) {
    BinOp_t *n;
    n = al.make_new<BinOp_t>();
    n->base.type = exprType::BinOp;
    n->m_op = type;
    n->m_left = x;
    n->m_right = y;
    return (PExpr)n;
}


static PExpr make_symbol(std::string s) {
    Name_t *n;
    n = al.make_new<Name_t>();
    n->base.type = exprType::Name;
    n->m_id = &s[0];
    return (PExpr)n;
}

static PExpr make_integer(std::string s) {
    Num_t *n;
    n = al.make_new<Num_t>();
    n->base.type = exprType::Num;
    n->m_n = s[0];
    return (PExpr)n;
}

/*


template <class Derived>
class BaseWalkVisitor
{
public:
    void visit(const BinOp &x) {
        apply(*x.left);
        apply(*x.right);
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void visit(const Pow &x) {
        apply(*x.base);
        apply(*x.exp);
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void visit(const Symbol &x) {
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void visit(const Integer &x) {
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void apply(const Node &b) {
        accept(b, *this);
    }
};

template <class Derived>
static void accept(const Node &x, BaseWalkVisitor<Derived> &v) {
    switch (x.type) {
        case nt_BinOp: { v.visit((const BinOp &)x); return; }
        case nt_Pow: { v.visit((const Pow &)x); return; }
        case nt_Symbol: { v.visit((const Symbol &)x); return; }
        case nt_Integer: { v.visit((const Integer &)x); return; }
    }
}

class CountVisitor : public BaseWalkVisitor<CountVisitor>
{
    int c_;
public:
    CountVisitor() : c_{0} {}
    template <typename T> void bvisit(const T &x) { }
    void bvisit(const Symbol &x) { c_ += 1; }
    int get_count() {
        return c_;
    }
};

static int count(const Node &b) {
    CountVisitor v;
    v.apply(b);
    return v.get_count();
}
*/

#define TYPE PExpr
#define ADD(x, y) make_binop(operatorType::Add, x, y)
#define SUB(x, y) make_binop(operatorType::Sub, x, y)
#define MUL(x, y) make_binop(operatorType::Mul, x, y)
#define DIV(x, y) make_binop(operatorType::Div, x, y)
#define POW(x, y) make_binop(operatorType::Pow, x, y)
#define SYMBOL(x) make_symbol(x)
#define INTEGER(x) make_integer(x)
#define PRINT(x) std::cout << x->type << std::endl
//#define PRINT(x) std::cout << count(*x) << std::endl;

#endif
