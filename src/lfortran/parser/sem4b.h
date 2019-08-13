#ifndef LFORTRAN_PARSER_SEM4b_H
#define LFORTRAN_PARSER_SEM4b_H

#include <lfortran/casts.h>
#include <lfortran/parser/alloc.h>

namespace LFortran {

extern Allocator al;

}

using LFortran::al;


enum NodeType
{
    nt_BinOp, nt_Pow, nt_Symbol, nt_Integer
};

enum BinOpType
{
    Add, Sub, Mul, Div
};

typedef struct Node *PNode;
struct Node {
    NodeType type;
/*
    union {
        BinOp binop;
        Pow pow;
        Symbol symbol;
        Integer integer;
    } d;
*/
};

struct BinOp {
    struct Node node;
    BinOpType btype;
    PNode left, right;
};

struct Pow {
    struct Node node;
    PNode base, exp;
};

struct Symbol {
    struct Node node;
    char *name;
};

struct Integer {
    struct Node node;
    char *i;
};



static Node* make_binop(BinOpType type, PNode x, PNode y) {
    struct BinOp *n;
    n = al.make_new<BinOp>();
    n->node.type = NodeType::nt_BinOp;
    n->btype = type;
    n->left = x;
    n->right = y;
    return (PNode)n;
}

static Node* make_pow(PNode x, PNode y) {
    struct Pow *n;
    n = al.make_new<Pow>();
    n->node.type = NodeType::nt_Pow;
    n->base = x;
    n->exp = y;
    return (PNode)n;
}

static Node* make_symbol(std::string s) {
    struct Symbol *n;
    n = al.make_new<Symbol>();
    n->node.type = NodeType::nt_Symbol;
    n->name = &s[0];
    return (PNode)n;
}

static Node* make_integer(std::string s) {
    struct Integer *n;
    n = al.make_new<Integer>();
    n->node.type = NodeType::nt_Integer;
    n->i = &s[0];
    return (PNode)n;
}



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


#define TYPE PNode
#define ADD(x, y) make_binop(BinOpType::Add, x, y)
#define SUB(x, y) make_binop(BinOpType::Sub, x, y)
#define MUL(x, y) make_binop(BinOpType::Mul, x, y)
#define DIV(x, y) make_binop(BinOpType::Div, x, y)
#define POW(x, y) make_pow(x, y)
#define SYMBOL(x) make_symbol(x)
#define INTEGER(x) make_integer(x)
//#define PRINT(x) std::cout << x->d.binop.right->type << std::endl
#define PRINT(x) std::cout << count(*x) << std::endl;


#endif
