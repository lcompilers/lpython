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
    return LFortran::AST::make_BinOp_t(al, x, type, y);
}


static PExpr make_symbol(std::string s) {
    return LFortran::AST::make_Name_t(al, &s[0]);
}

static PExpr make_integer(std::string s) {
    return LFortran::AST::make_Num_t(al, s[0]);
}



template <class Derived>
class BaseWalkVisitor
{
public:
    void visit(const BinOp_t &x) {
        apply(*x.m_left);
        apply(*x.m_right);
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void visit(const Name_t &x) {
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void visit(const Num_t &x) {
        LFortran::down_cast<Derived *>(this)->bvisit(x);
    }
    void apply(const expr_t &b) {
        accept(b, *this);
    }
};

template <class Derived>
static void accept(const expr_t &x, BaseWalkVisitor<Derived> &v) {
    switch (x.type) {
        case exprType::BinOp: { v.visit((const BinOp_t &)x); return; }
        case exprType::Name: { v.visit((const Name_t &)x); return; }
        case exprType::Num: { v.visit((const Num_t &)x); return; }
    }
}

class CountVisitor : public BaseWalkVisitor<CountVisitor>
{
    int c_;
public:
    CountVisitor() : c_{0} {}
    template <typename T> void bvisit(const T &x) { }
    void bvisit(const Name_t &x) { c_ += 1; }
    int get_count() {
        return c_;
    }
};

static int count(const expr_t &b) {
    CountVisitor v;
    v.apply(b);
    return v.get_count();
}

#define TYPE PExpr
#define ADD(x, y) make_binop(operatorType::Add, x, y)
#define SUB(x, y) make_binop(operatorType::Sub, x, y)
#define MUL(x, y) make_binop(operatorType::Mul, x, y)
#define DIV(x, y) make_binop(operatorType::Div, x, y)
#define POW(x, y) make_binop(operatorType::Pow, x, y)
#define SYMBOL(x) make_symbol(x)
#define INTEGER(x) make_integer(x)
//#define PRINT(x) std::cout << x->type << std::endl
#define PRINT(x) std::cout << count(*x) << std::endl;

#endif
