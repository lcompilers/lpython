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

#define EXPR(x) (down_cast<expr_t>(x))
#define EXPR2STMT(x) ((stmt_t*)make_Expr_t(p.m_a, x->base.loc, x))

#define RESULT(x) p.result.push_back(p.m_a, EXPR2STMT(EXPR(x)))


#define ADD(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Add, EXPR(y))
#define SUB(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Sub, EXPR(y))
#define MUL(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Mult, EXPR(y))
#define DIV(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Div, EXPR(y))
#define POW(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Pow, EXPR(y))
#define SYMBOL(x, l) make_Name_t(p.m_a, l, x.c_str(p.m_a), expr_contextType::Load)
// `x.int_n` is of type BigInt but we store the int64_t directly in AST
#define INTEGER(x, l) make_ConstantInt_t(p.m_a, l, x.int_n.n, nullptr)


#endif
