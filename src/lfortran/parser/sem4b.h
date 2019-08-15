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
using LFortran::AST::BoolOp_t;
using LFortran::AST::UnaryOp_t;
using LFortran::AST::Compare_t;
using LFortran::AST::FuncCall_t;
using LFortran::AST::FuncCallOrArray_t;
using LFortran::AST::Array_t;
using LFortran::AST::ArrayInitializer_t;
using LFortran::AST::Str_t;
using LFortran::AST::Constant_t;
typedef struct LFortran::AST::expr_t *PExpr;



static inline PExpr make_binop(operatorType type, PExpr x, PExpr y) {
    return LFortran::AST::make_BinOp_t(al, x, type, y);
}


static inline PExpr make_symbol(std::string s) {
    return LFortran::AST::make_Name_t(al, &s[0]);
}

static inline PExpr make_integer(std::string s) {
    return LFortran::AST::make_Num_t(al, s[0]);
}



template <class Derived>
class BaseVisitor
{
private:
    Derived& self() { return LFortran::down_cast<Derived&>(*this); }
public:
    void visit_expr(const expr_t &b) { visit_expr_t(b, self()); }
    void visit_BinOp(const BinOp_t &x) { }
    void visit_Name(const Name_t &x) { }
    void visit_Num(const Num_t &x) { }
    void visit_BoolOp(const BoolOp_t &x) { }
    void visit_UnaryOp(const UnaryOp_t &x) { }
    void visit_Compare(const Compare_t &x) { }
    void visit_FuncCall(const FuncCall_t &x) { }
    void visit_FuncCallOrArray(const FuncCallOrArray_t &x) { }
    void visit_Array(const Array_t &x) { }
    void visit_ArrayInitializer(const ArrayInitializer_t &x) { }
    void visit_Str(const Str_t &x) { }
    void visit_Constant(const Constant_t &x) { }
};

template <class Derived>
class BaseWalkVisitor : public BaseVisitor<Derived>
{
public:
    void visit_BinOp(const BinOp_t &x) {
        this->visit_expr(*x.m_left);
        this->visit_expr(*x.m_right);
    }
};

class CountVisitor : public BaseWalkVisitor<CountVisitor>
{
    int c_;
public:
    CountVisitor() : c_{0} {}
    void visit_Name(const Name_t &x) { c_ += 1; }
    int get_count() {
        return c_;
    }
};

static inline int count(const expr_t &b) {
    CountVisitor v;
    v.visit_expr(b);
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
