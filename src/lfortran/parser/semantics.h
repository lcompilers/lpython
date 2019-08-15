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

using LFortran::AST::operatorType;
using LFortran::AST::expr_t;
using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;

#define TYPE expr_t*
#define ADD(x, y) make_BinOp_t(p.m_a, x, operatorType::Add, y)
#define SUB(x, y) make_BinOp_t(p.m_a, x, operatorType::Sub, y)
#define MUL(x, y) make_BinOp_t(p.m_a, x, operatorType::Mul, y)
#define DIV(x, y) make_BinOp_t(p.m_a, x, operatorType::Div, y)
#define POW(x, y) make_BinOp_t(p.m_a, x, operatorType::Pow, y)
#define SYMBOL(x) make_Name_t(p.m_a, &x[0])
#define INTEGER(x) make_Num_t(p.m_a, x[0])
#define RESULT(x) p.result = x

#endif
