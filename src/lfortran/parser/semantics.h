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
using LFortran::AST::operatorType;
using LFortran::AST::stmt_t;
using LFortran::AST::expr_t;
using LFortran::AST::ast_t;
using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;

static inline expr_t* EXPR(ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::expr);
    return (expr_t*)f;
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
#define RESULT(x) p.result = x

#endif
