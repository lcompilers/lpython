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
using LFortran::AST::unaryopType;
using LFortran::AST::boolopType;
using LFortran::AST::cmpopType;
using LFortran::AST::stmtType;

using LFortran::AST::ast_t;
using LFortran::AST::attribute_t;
using LFortran::AST::attribute_arg_t;
using LFortran::AST::decl_t;
using LFortran::AST::dimension_t;
using LFortran::AST::expr_t;
using LFortran::AST::stmt_t;
using LFortran::AST::unit_decl2_t;

using LFortran::AST::Assignment_t;
using LFortran::AST::Name_t;
using LFortran::AST::Num_t;

using LFortran::AST::make_BinOp_t;
using LFortran::AST::make_Attribute_t;
using LFortran::AST::make_Constant_t;
using LFortran::AST::make_DoLoop_t;
using LFortran::AST::make_Exit_t;
using LFortran::AST::make_Cycle_t;
using LFortran::AST::make_Print_t;
using LFortran::AST::make_Return_t;
using LFortran::AST::make_Stop_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;
using LFortran::AST::make_Str_t;
using LFortran::AST::make_Real_t;
using LFortran::AST::make_SubroutineCall_t;
using LFortran::AST::make_WhileLoop_t;
using LFortran::AST::make_FuncCallOrArray_t;


static inline expr_t* EXPR(const ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::expr);
    return (expr_t*)f;
}

static inline char* name2char(const ast_t *n)
{
    LFORTRAN_ASSERT(EXPR(n)->type == exprType::Name)
    char *s = ((Name_t*)n)->m_id;
    return s;
}

template <typename T, astType type>
static inline T** vec_cast(const YYSTYPE::VecAST &x) {
    T **s = (T**)x.p;
    for (size_t i=0; i < x.size(); i++) {
        LFORTRAN_ASSERT((s[i]->base.type == type))
    }
    return s;
}

#define VEC_CAST(x, type) vec_cast<type##_t, astType::type>(x)
#define DECLS(x) VEC_CAST(x, unit_decl2)
#define STMTS(x) VEC_CAST(x, stmt)
#define ATTRS(x) VEC_CAST(x, attribute)
#define EXPRS(x) VEC_CAST(x, expr)

static inline stmt_t** IFSTMTS(Allocator &al, ast_t* x)
{
    stmt_t **s = al.allocate<stmt_t*>();
    LFORTRAN_ASSERT(x->type == astType::stmt);
    *s = (stmt_t*)x;
    LFORTRAN_ASSERT((*s)->base.type == astType::stmt)
    return s;
}

static inline decl_t* DECL(Allocator &al, const YYSTYPE::VecDecl &x,
        const YYSTYPE::Str &type, const YYSTYPE::VecAST &attrs)
{
    decl_t *s = al.allocate<decl_t>(x.size());
    for (size_t i=0; i < x.size(); i++) {
        s[i] = x.p[i];
        s[i].m_sym_type = type.c_str(al);
        s[i].m_attrs = ATTRS(attrs);
        s[i].n_attrs = attrs.size();
    }
    return s;
}

static inline decl_t* DECL3(Allocator &al, ast_t* n,
        const YYSTYPE::VecDim *d, expr_t *e)
{
    decl_t *s = al.allocate<decl_t>();
    s->m_sym = name2char(n);
    s->m_sym_type = nullptr;
    if (d) {
        s->n_dims = d->size();
        s->m_dims = d->p;
    } else {
        s->n_dims = 0;
        s->m_dims = nullptr;
    }
    s->n_attrs = 0;
    s->m_attrs = nullptr;
    s->m_initializer = e;
    return s;
}

static inline dimension_t DIM1(expr_t *a, expr_t *b)
{
    dimension_t s;
    s.m_start = a;
    s.m_end = b;
    return s;
}

static inline attribute_arg_t* ATTR_ARG(Allocator &al, const YYSTYPE::Str arg)
{
    attribute_arg_t *s = al.allocate<attribute_arg_t>();
    s->m_arg = arg.c_str(al);
    return s;
}

#define TYPE ast_t*

// Assign last_* location to `a` from `b`
#define LLOC(a, b) a.last_line = b.last_line; a.last_column = b.last_column;

#define ADD(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Add, EXPR(y))
#define SUB(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Sub, EXPR(y))
#define MUL(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Mul, EXPR(y))
#define DIV(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Div, EXPR(y))
#define POW(x, y, l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Pow, EXPR(y))
#define UNARY_MINUS(x, l) make_UnaryOp_t(p.m_a, l, unaryopType::USub, EXPR(x))
#define UNARY_PLUS(x, l) make_UnaryOp_t(p.m_a, l, unaryopType::UAdd, EXPR(x))
#define TRUE(l) make_Constant_t(p.m_a, l, true)
#define FALSE(l) make_Constant_t(p.m_a, l, false)

#define STRCONCAT(x, y, l) x /* TODO: concacenate */

#define EQ(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::Eq, EXPR(y))
#define NE(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::NotEq, EXPR(y))
#define LT(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::Lt, EXPR(y))
#define LE(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::LtE, EXPR(y))
#define GT(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::Gt, EXPR(y))
#define GE(x, y, l)  make_Compare_t(p.m_a, l, EXPR(x), cmpopType::GtE, EXPR(y))

#define NOT(x, l) make_UnaryOp_t(p.m_a, l, unaryopType::Not, EXPR(x))
#define AND(x, y, l) make_BoolOp_t(p.m_a, l, EXPR(x), boolopType::And, EXPR(y))
#define OR(x, y, l)  make_BoolOp_t(p.m_a, l, EXPR(x), boolopType::Or,  EXPR(y))
#define EQV(x, y, l) make_BoolOp_t(p.m_a, l, EXPR(x), boolopType::Eqv, EXPR(y))
#define NEQV(x, y, l) make_BoolOp_t(p.m_a, l, EXPR(x), boolopType::NEqv, EXPR(y))

#define ARRAY_IN(a, l) make_ArrayInitializer_t(p.m_a, l, \
        EXPRS(a), a.size())

#define SYMBOL(x, l) make_Name_t(p.m_a, l, x.c_str(p.m_a));
#define INTEGER(x, l) make_Num_t(p.m_a, l, x)
#define REAL(x, l) make_Real_t(p.m_a, l, x.c_str(p.m_a))
#define STRING(x, l) make_Str_t(p.m_a, l, x.c_str(p.m_a))
#define ASSIGNMENT(x, y, l) make_Assignment_t(p.m_a, l, EXPR(x), EXPR(y))
#define ASSOCIATE(x, y, l) make_Associate_t(p.m_a, l, EXPR(x), EXPR(y))
#define CALL(x, l) make_SubroutineCall_t(p.m_a, l, \
        name2char(x), \
        nullptr, 0)

#define PRINT0(l) make_Print_t(p.m_a, l, nullptr, nullptr, 0)
#define PRINT(args, l) make_Print_t(p.m_a, l, nullptr, EXPRS(args), args.size())
#define PRINTF0(fmt, l) make_Print_t(p.m_a, l, fmt.c_str(p.m_a), nullptr, 0)
#define PRINTF(fmt, args, l) make_Print_t(p.m_a, l, fmt.c_str(p.m_a), \
        EXPRS(args), args.size())

#define WRITE0(u, l) make_Print_t(p.m_a, l, nullptr, nullptr, 0)
#define WRITE(u, args, l) make_Print_t(p.m_a, l, nullptr, \
        EXPRS(args), args.size())
#define WRITEF0(u, fmt, l) make_Print_t(p.m_a, l, fmt.c_str(p.m_a), nullptr, 0)
#define WRITEF(u, fmt, args, l) make_Print_t(p.m_a, l, fmt.c_str(p.m_a), \
        EXPRS(args), args.size())
#define WRITEE0(u, l) make_Print_t(p.m_a, l, nullptr, nullptr, 0)
#define WRITEE(u, args, l) make_Print_t(p.m_a, l, nullptr, \
        EXPRS(args), args.size())

#define STOP(l) make_Stop_t(p.m_a, l, nullptr)
#define STOP1(e, l) make_Stop_t(p.m_a, l, EXPR(e))

#define EXIT(l) make_Exit_t(p.m_a, l)
#define RETURN(l) make_Return_t(p.m_a, l)
#define CYCLE(l) make_Cycle_t(p.m_a, l)
#define SUBROUTINE(name, decl, stmts, l) make_Subroutine_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define FUNCTION(name, decl, stmts, l) make_Function_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        /*return_type*/ nullptr, \
        /*return_var*/ nullptr, \
        /*bind*/ nullptr, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define PROGRAM(name, decl, stmts, l) make_Program_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define RESULT(x) p.result.push_back(x)

#define IFSINGLE(cond, body, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ IFSTMTS(p.m_a, body), \
        /*n_body*/ 1, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF1(cond, body, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF2(cond, body, orelse, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ STMTS(orelse), \
        /*n_orelse*/ orelse.size())

#define IF3(cond, body, ifblock, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ IFSTMTS(p.m_a, ifblock), \
        /*n_orelse*/ 1)

#define WHERESINGLE(cond, body, l) make_Where_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ IFSTMTS(p.m_a, body), \
        /*n_body*/ 1, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define WHERE1(cond, body, l) make_Where_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define WHERE2(cond, body, orelse, l) make_Where_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ STMTS(orelse), \
        /*n_orelse*/ orelse.size())

#define WHERE3(cond, body, whereblock, l) make_Where_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ IFSTMTS(p.m_a, whereblock), \
        /*n_orelse*/ 1)

#define LIST_NEW(l) l.reserve(p.m_a, 4)
#define LIST_ADD(l, x) l.push_back(p.m_a, x)
#define PLIST_ADD(l, x) l.push_back(p.m_a, *x)

#define WHILE(cond, body, l) make_WhileLoop_t(p.m_a, l, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO1(body, l) make_DoLoop_t(p.m_a, l, \
        nullptr, nullptr, nullptr, nullptr, \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO2(i, a, b, body, l) make_DoLoop_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), nullptr, \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO3(i, a, b, c, body, l) make_DoLoop_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), EXPR(c), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define VAR_DECL(type, attrs, syms, l) make_Declaration_t(p.m_a, l, \
        DECL(p.m_a, syms, type, attrs), syms.size())

#define VAR_SYM_DECL1(id, l)         DECL3(p.m_a, id, nullptr, nullptr)
#define VAR_SYM_DECL2(id, e, l)      DECL3(p.m_a, id, nullptr, EXPR(e))
#define VAR_SYM_DECL3(id, a, l)      DECL3(p.m_a, id, &a, nullptr)
#define VAR_SYM_DECL4(id, a, e, l)   DECL3(p.m_a, id, &a, EXPR(e))

#define ARRAY_COMP_DECL1(a, l)       DIM1(EXPR(INTEGER(1, l)), EXPR(a))
#define ARRAY_COMP_DECL2(a, b, l)    DIM1(EXPR(a), EXPR(b))
#define ARRAY_COMP_DECL3(a, l)       DIM1(EXPR(a), nullptr)
#define ARRAY_COMP_DECL4(b, l)       DIM1(nullptr, EXPR(b))
#define ARRAY_COMP_DECL5(l)          DIM1(nullptr, nullptr)

#define VARMOD(a, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ nullptr, \
        /*n_args*/ 0)

#define VARMOD2(a, b, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ ATTR_ARG(p.m_a, b), \
        /*n_args*/ 1)

#define FUNCCALLORARRAY(id, l) make_FuncCallOrArray_t(p.m_a, l, \
        /*char* a_func*/ name2char(id), \
        /*expr_t** a_args*/ nullptr, /*size_t n_args*/ 0, \
        /*keyword_t* a_keywords*/ nullptr, /*size_t n_keywords*/ 0)

#endif
