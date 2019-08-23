#ifndef LFORTRAN_PARSER_SEM4b_H
#define LFORTRAN_PARSER_SEM4b_H

/*
   This header file contains parser semantics: how the AST classes get
   constructed from the parser. This file only gets included in the generated
   parser cpp file, nowhere else.

   Note that this is part of constructing the AST from the source code, not the
   LFortran semantic phase (AST -> ASR).
*/

#include <cstring>

#include <lfortran/ast.h>

using LFortran::Location;

using LFortran::AST::astType;
using LFortran::AST::exprType;
using LFortran::AST::operatorType;
using LFortran::AST::stmtType;

using LFortran::AST::ast_t;
using LFortran::AST::do_loop_head_t;
using LFortran::AST::expr_t;
using LFortran::AST::stmt_t;

using LFortran::AST::Assignment_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;

using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_DoLoop_t;
using LFortran::AST::make_Exit_t;
using LFortran::AST::make_Cycle_t;
using LFortran::AST::make_Return_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;
using LFortran::AST::make_WhileLoop_t;


static inline expr_t* EXPR(const ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::expr);
    return (expr_t*)f;
}

static inline do_loop_head_t DOLOOP_HEAD(const expr_t *i, expr_t *a,
        expr_t *b, expr_t *c)
{
    do_loop_head_t s;
    if (i) {
        LFORTRAN_ASSERT(i->type == exprType::Name)
        s.m_var = ((Name_t*)i)->m_id;
    } else {
        s.m_var = nullptr;
    }
    s.m_start = a;
    s.m_end = b;
    s.m_increment = c;
    return s;
}

static inline stmt_t** STMTS(Allocator &al, const YYSTYPE::Vec &x)
{
    stmt_t **s = (stmt_t**)x.p;
    for (size_t i=0; i < x.n; i++) {
        LFORTRAN_ASSERT(s[i]->base.type == astType::stmt)
    }
    return s;
}

static inline stmt_t** IFSTMTS(Allocator &al, ast_t* x)
{
    stmt_t **s = (stmt_t**)al.allocate(sizeof(stmt_t*) * 1);
    LFORTRAN_ASSERT(x->type == astType::stmt);
    *s = (stmt_t*)x;
    LFORTRAN_ASSERT((*s)->base.type == astType::stmt)
    return s;
}

static inline char* name2char(const expr_t *n)
{
    LFORTRAN_ASSERT(n->type == exprType::Name)
    char *s = ((Name_t*)n)->m_id;
    return s;
}

static inline ast_t* make_SYMBOL(Allocator &al, const Location &loc,
        const YYSTYPE::Str &x)
{
    // Copy the string into our own allocated memory.
    // `x` is not NULL terminated, but we need to make it null terminated.
    // TODO: Instead, we should pass a pointer to the Tokenizer's string of the
    // original source code (the string there is not zero terminated, as it's a
    // substring), and a length. And provide functions to deal with the
    // non-zero terminated string properly. That will be much faster.
    char *s = x.c_str(al);
    LFORTRAN_ASSERT(s[x.n] == '\0');
    return make_Name_t(al, loc, s);
}


#define TYPE ast_t*
#define ADD(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Add, EXPR(y))
#define SUB(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Sub, EXPR(y))
#define MUL(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Mul, EXPR(y))
#define DIV(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Div, EXPR(y))
#define POW(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Pow, EXPR(y))
#define SYMBOL(x, l) make_SYMBOL(p.m_a, l, x)
#define INTEGER(x, l) make_Num_t(p.m_a, l, x)
#define ASSIGNMENT(x, y, l) make_Assignment_t(p.m_a, l, EXPR(x), EXPR(y))
#define EXIT(l) make_Exit_t(p.m_a, l)
#define RETURN(l) make_Return_t(p.m_a, l)
#define CYCLE(l) make_Cycle_t(p.m_a, l)
#define SUBROUTINE(name, stmts, l) make_Subroutine_t(p.m_a, l, \
        /*name*/ nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ nullptr, \
        /*n_decl*/ 0, \
        /*body*/ STMTS(p.m_a, stmts), \
        /*n_body*/ stmts.n, \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define FUNCTION(name, stmts, l) make_Function_t(p.m_a, l, \
        /*name*/ nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        /*return_type*/ nullptr, \
        /*return_var*/ nullptr, \
        /*bind*/ nullptr, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ nullptr, \
        /*n_decl*/ 0, \
        /*body*/ STMTS(p.m_a, stmts), \
        /*n_body*/ stmts.n, \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define PROGRAM(name, stmts, l) make_Program_t(p.m_a, l, \
        /*name*/ name2char(EXPR(name)), \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ nullptr, \
        /*n_decl*/ 0, \
        /*body*/ STMTS(p.m_a, stmts), \
        /*n_body*/ stmts.n, \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define RESULT(x) p.result.push_back(x)

#define IF1(cond, body, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF2(cond, body, orelse, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n, \
        /*a_orelse*/ STMTS(p.m_a, orelse), \
        /*n_orelse*/ orelse.n)

#define IF3(cond, body, ifblock, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n, \
        /*a_orelse*/ IFSTMTS(p.m_a, ifblock), \
        /*n_orelse*/ 1)

#define STMTS_NEW(l) l.reserve(p.m_a, 4)
#define STMTS_ADD(l, x) l.push_back(p.m_a, x)

#define WHILE(cond, body, l) make_WhileLoop_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n)

#define DO1(body, l) make_DoLoop_t(p.m_a, l, \
        /*head*/ DOLOOP_HEAD(nullptr, nullptr, nullptr, nullptr), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n)

#define DO2(i, a, b, body, l) make_DoLoop_t(p.m_a, l, \
        /*head*/ DOLOOP_HEAD(EXPR(i), EXPR(a), EXPR(b), nullptr), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n)

#define DO3(i, a, b, c, body, l) make_DoLoop_t(p.m_a, l, \
        /*head*/ DOLOOP_HEAD(EXPR(i), EXPR(a), EXPR(b), EXPR(c)), \
        /*body*/ STMTS(p.m_a, body), \
        /*n_body*/ body.n)

#endif
