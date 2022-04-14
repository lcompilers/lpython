#ifndef LPYTHON_PARSER_SEMANTICS_H
#define LPYTHON_PARSER_SEMANTICS_H

/*
   This header file contains parser semantics: how the AST classes get
   constructed from the parser. This file only gets included in the generated
   parser cpp file, nowhere else.

   Note that this is part of constructing the AST from the source code, not the
   LPython semantic phase (AST -> ASR).
*/

#include <cstring>

#include <lpython/python_ast.h>
#include <libasr/string_utils.h>

// This is only used in parser.tab.cc, nowhere else, so we simply include
// everything from LFortran::AST to save typing:
using namespace LFortran::LPython::AST;
using LFortran::Location;
using LFortran::Vec;

static inline char* name2char(const ast_t *n) {
    return down_cast2<Name_t>(n)->m_id;
}

Vec<ast_t*> A2LIST(Allocator &al, ast_t *x) {
    Vec<ast_t*> v;
    v.reserve(al, 1);
    v.push_back(al, x);
    return v;
}

static inline char** REDUCE_ARGS(Allocator &al, const Vec<ast_t*> args) {
    char **a = al.allocate<char*>(args.size());
    for (size_t i=0; i < args.size(); i++) {
        a[i] = name2char(args.p[i]);
    }
    return a;
}

template <typename T, astType type>
static inline T** vec_cast(const Vec<ast_t*> &x) {
    T **s = (T**)x.p;
    for (size_t i=0; i < x.size(); i++) {
        LFORTRAN_ASSERT((s[i]->base.type == type))
    }
    return s;
}

#define VEC_CAST(x, type) vec_cast<type##_t, astType::type>(x)
#define STMTS(x) VEC_CAST(x, stmt)
#define EXPRS(x) VEC_CAST(x, expr)

#define EXPR(x) (down_cast<expr_t>(x))
#define STMT(x) (down_cast<stmt_t>(x))
#define EXPR2STMT(x) ((stmt_t*)make_Expr_t(p.m_a, x->base.loc, x))

#define RESULT(x) p.result.push_back(p.m_a, EXPR2STMT(EXPR(x)))
#define LIST_NEW(l) l.reserve(p.m_a, 4)
#define LIST_ADD(l, x) l.push_back(p.m_a, x)
#define PLIST_ADD(l, x) l.push_back(p.m_a, *x)

#define PASS(l) make_Pass_t(p.m_a, l)
#define BREAK(l) make_Break_t(p.m_a, l)
#define CONTINUE(l) make_Continue_t(p.m_a, l)

#define RAISE_01(l) make_Raise_t(p.m_a, l, nullptr, nullptr)
#define RAISE_02(exec, l) make_Raise_t(p.m_a, l, EXPR(exec), nullptr)
#define RAISE_03(exec, cause, l) make_Raise_t(p.m_a, l, EXPR(exec), EXPR(cause))

#define ASSERT_01(test, l) make_Assert_t(p.m_a, l, EXPR(test), nullptr)
#define ASSERT_02(test, msg, l) make_Assert_t(p.m_a, l, EXPR(test), EXPR(msg))

#define GLOBAL(names, l) make_Global_t(p.m_a, l, \
        REDUCE_ARGS(p.m_a, names), names.size())

#define NON_LOCAL(names, l) make_Nonlocal_t(p.m_a, l, \
        REDUCE_ARGS(p.m_a, names), names.size())

#define BINOP(x, op, y, l) make_BinOp_t(p.m_a, l, \
        EXPR(x), operatorType::op, EXPR(y))
#define COMPARE(x, op, y, l) make_Compare_t(p.m_a, l, \
        EXPR(x), cmpopType::op, EXPRS(A2LIST(p.m_a, y)), 1)

#define SYMBOL(x, l) make_Name_t(p.m_a, l, \
        x.c_str(p.m_a), expr_contextType::Load)
// `x.int_n` is of type BigInt but we store the int64_t directly in AST
#define INTEGER(x, l) make_ConstantInt_t(p.m_a, l, x.int_n.n, nullptr)
#define STRING(x, l) make_ConstantStr_t(p.m_a, l, x.c_str(p.m_a), nullptr)
#define FLOAT(x, l) make_ConstantFloat_t(p.m_a, l, \
        std::stof(x.c_str(p.m_a)), nullptr)


#endif
