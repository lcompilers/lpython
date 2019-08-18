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
using LFortran::AST::stmtType;
using LFortran::AST::operatorType;
using LFortran::AST::stmt_t;
using LFortran::AST::expr_t;
using LFortran::AST::ast_t;
using LFortran::AST::Assignment_t;
using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;

static inline expr_t* EXPR(ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::expr);
    return (expr_t*)f;
}

static inline stmt_t** STMTS(Allocator &al, std::vector<ast_t*> &x)
{
    stmt_t **s = (stmt_t**)al.allocate(sizeof(stmt_t*) * x.size());
    for (size_t i=0; i < x.size(); i++) {
        LFORTRAN_ASSERT(x[i]->type == astType::stmt);
        s[i] = (stmt_t*)x[i];
    }
    for (size_t i=0; i < x.size(); i++) {
        LFORTRAN_ASSERT(s[i]->base.type == astType::stmt)
    }
    return s;
}


#define TYPE ast_t*
#define ADD(x, y) make_BinOp_t(p.m_a, EXPR(x), operatorType::Add, EXPR(y))
#define SUB(x, y) make_BinOp_t(p.m_a, EXPR(x), operatorType::Sub, EXPR(y))
#define MUL(x, y) make_BinOp_t(p.m_a, EXPR(x), operatorType::Mul, EXPR(y))
#define DIV(x, y) make_BinOp_t(p.m_a, EXPR(x), operatorType::Div, EXPR(y))
#define POW(x, y) make_BinOp_t(p.m_a, EXPR(x), operatorType::Pow, EXPR(y))
#define SYMBOL(x) make_Name_t(p.m_a, &x[0])
#define INTEGER(x) make_Num_t(p.m_a, x)
#define ASSIGNMENT(x, y) make_Assignment_t(p.m_a, EXPR(x), EXPR(y))
#define SUBROUTINE(x) make_Subroutine_t(p.m_a, \
        /*name*/ nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ nullptr, \
        /*n_decl*/ 0, \
        /*body*/ STMTS(p.m_a, x), \
        /*n_body*/ x.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define RESULT(x) p.result = x

#endif
