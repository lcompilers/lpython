#ifndef LFORTRAN_PARSER_SEM4b_H
#define LFORTRAN_PARSER_SEM4b_H

/*
   This header file contains parser semantics: how the AST classes get
   constructed from the parser. This file only gets included in the generated
   parser cpp file, nowhere else.

   Note that this is part of constructing the AST from the source code, not the
   LFortran semantic phase (AST -> ASR).
*/

#include <lfortran/ast.h>

using LFortran::AST::astType;
using LFortran::Location;
using LFortran::AST::stmtType;
using LFortran::AST::operatorType;
using LFortran::AST::stmt_t;
using LFortran::AST::expr_t;
using LFortran::AST::ast_t;
using LFortran::AST::Num_t;
using LFortran::AST::Assignment_t;
using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_Exit_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;

static inline expr_t* EXPR(const ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::expr);
    return (expr_t*)f;
}

static inline stmt_t** STMTS(Allocator &al, const YYSTYPE::Vec &x)
{
    stmt_t **s = (stmt_t**)al.allocate(sizeof(stmt_t*) * x.n);
    for (size_t i=0; i < x.n; i++) {
        LFORTRAN_ASSERT(x.p[i]->type == astType::stmt);
        s[i] = (stmt_t*)x.p[i];
    }
    for (size_t i=0; i < x.n; i++) {
        LFORTRAN_ASSERT(s[i]->base.type == astType::stmt)
    }
    return s;
}

static inline ast_t* make_SYMBOL(Allocator &al, const Location &loc,
        const std::string &x)
{
    // Copy the string into our own allocated memory.
    // TODO: Instead, we should pass a pointer to the Tokenizer's string of the
    // original source code (the string there is not zero terminated, as it's a
    // substring), and a length. And provide functions to deal with the
    // non-zero terminated string properly. That will be much faster.
    char *s = (char *)al.allocate(sizeof(char) * (x.size()+1));
    for (size_t i=0; i < x.size()+1; i++) {
        s[i] = x[i];
    }
    LFORTRAN_ASSERT(s[x.size()] == '\0');
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
#define EXIT() make_Exit_t(p.m_a, l)
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
        /*name*/ nullptr, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ nullptr, \
        /*n_decl*/ 0, \
        /*body*/ STMTS(p.m_a, stmts), \
        /*n_body*/ stmts.n, \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define RESULT(x) p.result.push_back(x)

#define STMTS_NEW(l) { \
    l.n = 0; \
    l.p = (ast_t**)p.m_a.allocate(sizeof(ast_t*) * 100); \
}
#define STMTS_ADD(l, x) {l.p[l.n] = x; l.n++;}

#endif
