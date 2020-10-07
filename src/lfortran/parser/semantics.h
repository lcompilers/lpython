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

using LFortran::AST::down_cast;
using LFortran::AST::down_cast2;

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
using LFortran::AST::arg_t;
using LFortran::AST::case_stmt_t;
using LFortran::AST::use_symbol_t;
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
using LFortran::AST::make_ErrorStop_t;
using LFortran::AST::make_Name_t;
using LFortran::AST::make_Num_t;
using LFortran::AST::make_Str_t;
using LFortran::AST::make_Real_t;
using LFortran::AST::make_SubroutineCall_t;
using LFortran::AST::make_WhileLoop_t;
using LFortran::AST::make_FuncCallOrArray_t;
using LFortran::AST::make_Use_t;
using LFortran::AST::make_UseSymbol_t;
using LFortran::AST::make_Module_t;
using LFortran::AST::make_Private_t;
using LFortran::AST::make_Public_t;
using LFortran::AST::make_Interface_t;


static inline expr_t* EXPR(const ast_t *f)
{
    return down_cast<expr_t>(f);
}

static inline attribute_t* ATTR(const ast_t *f)
{
    return down_cast<attribute_t>(f);
}

static inline expr_t* EXPR_OPT(const ast_t *f)
{
    if (f) {
        return EXPR(f);
    } else {
        return nullptr;
    }
}

static inline char* name2char(const ast_t *n)
{
    return down_cast2<Name_t>(n)->m_id;
}

template <typename T, astType type>
static inline T** vec_cast(const LFortran::Vec<ast_t*> &x) {
    T **s = (T**)x.p;
    for (size_t i=0; i < x.size(); i++) {
        LFORTRAN_ASSERT((s[i]->base.type == type))
    }
    return s;
}

#define VEC_CAST(x, type) vec_cast<LFortran::AST::type##_t, astType::type>(x)
#define DECLS(x) VEC_CAST(x, unit_decl2)
#define USES(x) VEC_CAST(x, unit_decl1)
#define STMTS(x) VEC_CAST(x, stmt)
#define CONTAINS(x) VEC_CAST(x, program_unit)
#define ATTRS(x) VEC_CAST(x, attribute)
#define EXPRS(x) VEC_CAST(x, expr)
#define CASE_STMTS(x) VEC_CAST(x, case_stmt)
#define USE_SYMBOLS(x) VEC_CAST(x, use_symbol)
#define CONCURRENT_CONTROLS(x) VEC_CAST(x, concurrent_control)
#define CONCURRENT_LOCALITIES(x) VEC_CAST(x, concurrent_locality)

static inline stmt_t** IFSTMTS(Allocator &al, ast_t* x)
{
    stmt_t **s = al.allocate<stmt_t*>();
    *s = down_cast<stmt_t>(x);
    return s;
}

static inline LFortran::AST::kind_item_t *make_kind_item_t(Allocator &al,
    Location &loc, char *id, LFortran::AST::ast_t *value, LFortran::AST::kind_item_typeType type)
{
    LFortran::AST::kind_item_t *r = al.allocate<LFortran::AST::kind_item_t>(1);
    r->loc = loc;
    r->m_id = id;
    if (value) {
        r->m_value = down_cast<LFortran::AST::expr_t>(value);
    } else {
        r->m_value = nullptr;
    }
    r->m_type = type;
    return r;
}

#define KIND_ARG1(k, l) make_kind_item_t(p.m_a, l, nullptr, k, \
        LFortran::AST::kind_item_typeType::Value)
#define KIND_ARG1S(l) make_kind_item_t(p.m_a, l, nullptr, nullptr, \
        LFortran::AST::kind_item_typeType::Star)
#define KIND_ARG1C(l) make_kind_item_t(p.m_a, l, nullptr, nullptr, \
        LFortran::AST::kind_item_typeType::Colon)
#define KIND_ARG2(id, k, l) make_kind_item_t(p.m_a, l, name2char(id), k, \
        LFortran::AST::kind_item_typeType::Value)
#define KIND_ARG2S(id, l) make_kind_item_t(p.m_a, l, name2char(id), nullptr, \
        LFortran::AST::kind_item_typeType::Star)
#define KIND_ARG2C(id, l) make_kind_item_t(p.m_a, l, name2char(id), nullptr, \
        LFortran::AST::kind_item_typeType::Colon)

static inline decl_t* DECL(Allocator &al, const LFortran::Vec<decl_t> &x,
        char *type, LFortran::Vec<LFortran::AST::kind_item_t> kind,
        const LFortran::Vec<ast_t*> &attrs)
{
    decl_t *s = al.allocate<decl_t>(x.size());
    for (size_t i=0; i < x.size(); i++) {
        s[i] = x.p[i];
        s[i].m_sym_type = type;
        s[i].n_kind = kind.size();
        s[i].m_kind = kind.p;
        s[i].m_attrs = ATTRS(attrs);
        s[i].n_attrs = attrs.size();
    }
    return s;
}

static inline decl_t* DECL2(Allocator &al, const LFortran::Vec<decl_t> &x,
        char *type, const ast_t *attr)
{
    decl_t *s = al.allocate<decl_t>(x.size());
    attribute_t **a = al.allocate<attribute_t*>(1);
    a[0] = ATTR(attr);
    for (size_t i=0; i < x.size(); i++) {
        s[i] = x.p[i];
        s[i].m_sym_type = type;
        s[i].m_attrs = a;
        s[i].n_attrs = 1;
    }
    return s;
}

static inline decl_t* DECL2b(Allocator &al, const ast_t *attr)
{
    decl_t *s = al.allocate<decl_t>(1);
    attribute_t **a = al.allocate<attribute_t*>(1);
    a[0] = ATTR(attr);

    s[0].m_sym = nullptr;
    s[0].m_sym_type = nullptr;
    s[0].m_dims = nullptr;
    s[0].n_dims = 0;
    s[0].m_attrs = a;
    s[0].n_attrs = 1;
    s[0].m_initializer = nullptr;
    return s;
}

static inline decl_t* DECL2c(Allocator &al, Location &l)
{
    decl_t *s = al.allocate<decl_t>(1);
    s[0].loc = l;
    s[0].m_sym = nullptr;
    s[0].m_sym_type = nullptr;
    s[0].m_dims = nullptr;
    s[0].n_dims = 0;
    s[0].m_attrs = nullptr;
    s[0].n_attrs = 0;
    s[0].m_initializer = nullptr;
    return s;
}

static inline decl_t* DECL3(Allocator &al, ast_t* n,
        const LFortran::Vec<dimension_t> *d, expr_t *e)
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

static inline expr_t** DIMS2EXPRS(Allocator &al, const LFortran::Vec<LFortran::FnArg> &d)
{
    if (d.size() == 0) {
        return nullptr;
    } else {
        expr_t **s = al.allocate<expr_t*>(d.size());
        for (size_t i=0; i < d.size(); i++) {
            // TODO: we need to change this to allow both array and fn arguments
            // Right now we assume everything is a function argument
            if (d[i].keyword) {
                if (d[i].kw.m_value.m_end) {
                    s[i] = d[i].kw.m_value.m_end;
                } else {
                    Location l;
                    s[i] = EXPR(make_Num_t(al, l, 1));
                }
            } else {
                if (d[i].arg.m_end) {
                    s[i] = d[i].arg.m_end;
                } else {
                    Location l;
                    s[i] = EXPR(make_Num_t(al, l, 1));
                }
            }
        }
        return s;
    }
}

static inline LFortran::Vec<LFortran::AST::kind_item_t> empty()
{
    LFortran::Vec<LFortran::AST::kind_item_t> r;
    r.from_pointer_n(nullptr, 0);
    return r;
}

static inline LFortran::VarType* VARTYPE0_(Allocator &al,
        const LFortran::Str &s, const LFortran::Vec<LFortran::AST::kind_item_t> kind, Location &l)
{
    LFortran::VarType *r = al.allocate<LFortran::VarType>(1);
    r->loc = l;
    r->string = s;
    r->kind = kind;
    return r;
}

#define VARTYPE0(s, l) VARTYPE0_(p.m_a, s, empty(), l)
#define VARTYPE3(s, k, l) VARTYPE0_(p.m_a, s, k, l)

static inline LFortran::FnArg* DIM1(Allocator &al, Location &l, expr_t *a, expr_t *b)
{
    LFortran::FnArg *s = al.allocate<LFortran::FnArg>();
    s->keyword = false;
    s->arg.loc = l;
    s->arg.m_start = a;
    s->arg.m_end = b;
    s->arg.m_step = nullptr;
    return s;
}

static inline LFortran::FnArg* DIM1k(Allocator &al, Location &l,
        ast_t *id, expr_t *a, expr_t *b)
{
    LFortran::FnArg *s = al.allocate<LFortran::FnArg>();
    s->keyword = true;
    s->kw.loc = l;
    s->kw.m_arg = name2char(id);
    s->kw.m_value.m_start = a;
    s->kw.m_value.m_end = b;
    s->kw.m_value.m_step = nullptr;
    return s;
}

static inline dimension_t DIM1d(Location &l, expr_t *a, expr_t *b)
{
    dimension_t s;
    s.loc = l;
    s.m_start = a;
    s.m_end = b;
    return s;
}

// TODO: generate this, and DIM1 above and others automatically
static inline LFortran::AST::parameter_item_t make_parameter_item_t(Location &l,
    char *name, expr_t *value)
{
    LFortran::AST::parameter_item_t s;
    s.loc = l;
    s.m_name = name;
    s.m_value = value;
    return s;
}

static inline attribute_arg_t* ATTR_ARG(Allocator &al, Location &l,
        const LFortran::Str arg)
{
    attribute_arg_t *s = al.allocate<attribute_arg_t>();
    s->loc = l;
    s->m_arg = arg.c_str(al);
    return s;
}

static inline arg_t* ARGS(Allocator &al, Location &l,
    const LFortran::Vec<ast_t*> args)
{
    arg_t *a = al.allocate<arg_t>(args.size());
    for (size_t i=0; i < args.size(); i++) {
        a[i].loc = l;
        a[i].m_arg = name2char(args.p[i]);
    }
    return a;
}

/*
static inline LFortran::AST::reduce_t* REDUCE_TYPE(const ast_t *f)
{
    LFORTRAN_ASSERT(f->type == astType::reduce);
    return (LFortran::AST::reduce_t*)f;
}
*/

static inline char** REDUCE_ARGS(Allocator &al, const LFortran::Vec<ast_t*> args)
{
    char **a = al.allocate<char*>(args.size());
    for (size_t i=0; i < args.size(); i++) {
        a[i] = name2char(args.p[i]);
    }
    return a;
}

static inline LFortran::AST::reduce_opType convert_id_to_reduce_type(
        const Location &loc, const ast_t *id)
{
        std::string s_id = down_cast2<Name_t>(id)->m_id;
        if (s_id == "MIN" ) {
                return LFortran::AST::reduce_opType::ReduceMIN;
        } else if (s_id == "MAX") {
                return LFortran::AST::reduce_opType::ReduceMAX;
        } else {
                throw LFortran::SemanticError("Unsupported operation in reduction", loc);
        }
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
ast_t* SUBROUTINE_CALL0(Allocator &al, const ast_t *id,
        const LFortran::Vec<LFortran::FnArg> &args, Location &l) {
    LFortran::Vec<LFortran::AST::fnarg_t> v;
    v.reserve(al, args.size());
    LFortran::Vec<LFortran::AST::keyword_t> v2;
    v2.reserve(al, args.size());
    for (auto &item : args) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_SubroutineCall_t(al, l,
        /*char* a_func*/ name2char(id),
        /*expr_t** a_args*/ v.p, /*size_t n_args*/ v.size(),
        /*keyword_t* a_keywords*/ v2.p, /*size_t n_keywords*/ v2.size());
}
#define SUBROUTINE_CALL(name, args, l) SUBROUTINE_CALL0(p.m_a, \
        name, args, l)
#define SUBROUTINE_CALL2(name, l) make_SubroutineCall_t(p.m_a, l, \
        name2char(name), \
        nullptr, 0, nullptr, 0)

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
#define ERROR_STOP(l) make_ErrorStop_t(p.m_a, l, nullptr)
#define ERROR_STOP1(e, l) make_ErrorStop_t(p.m_a, l, EXPR(e))

#define EXIT(l) make_Exit_t(p.m_a, l)
#define RETURN(l) make_Return_t(p.m_a, l)
#define CYCLE(l) make_Cycle_t(p.m_a, l)
#define SUBROUTINE(name, args, decl, stmts, l) make_Subroutine_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)

char *str_or_null(Allocator &al, const LFortran::Str &s) {
    if (s.size() == 0) {
        return nullptr;
    } else {
        return s.c_str(al);
    }
}

char *fn_type2return_type(const LFortran::Vec<ast_t*> &v) {
    for (size_t i=0; i < v.size(); i++) {
        if (v[i]->type == LFortran::AST::astType::fn_mod) {
            LFortran::AST::FnMod_t *t = (LFortran::AST::FnMod_t*)v[i];
            return t->m_s;
        }
    }
    return nullptr;
}

#define FUNCTION(fn_type, name, args, return_var, decl, stmts, l) make_Function_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*return_type*/ fn_type2return_type(fn_type), \
        /*return_var*/ EXPR_OPT(return_var), \
        /*bind*/ nullptr, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define FUNCTION0(name, args, return_var, decl, stmts, l) make_Function_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*return_type*/ nullptr, \
        /*return_var*/ EXPR_OPT(return_var), \
        /*bind*/ nullptr, \
        /*use*/ nullptr, \
        /*n_use*/ 0, \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ nullptr, \
        /*n_contains*/ 0)
#define PROGRAM(name, use, decl, stmts, contains, l) make_Program_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())
#define RESULT(x) p.result.push_back(p.m_a, x)

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

#define DO_CONCURRENT1(h, loc, body, l) make_DoConcurrentLoop_t(p.m_a, l, \
        CONCURRENT_CONTROLS(h), h.size(), \
        nullptr, \
        CONCURRENT_LOCALITIES(loc), loc.size(), \
        STMTS(body), body.size())

#define DO_CONCURRENT2(h, m, loc, body, l) make_DoConcurrentLoop_t(p.m_a, l, \
        CONCURRENT_CONTROLS(h), h.size(), \
        EXPR(m), \
        CONCURRENT_LOCALITIES(loc), loc.size(), \
        STMTS(body), body.size())


#define DO_CONCURRENT_REDUCE(i, a, b, reduce, body, l) make_DoConcurrentLoop_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), nullptr, \
        /*reduce*/ REDUCE_TYPE(reduce), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define CONCURRENT_CONTROL1(i, a, b, l) LFortran::AST::make_ConcurrentControl_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), nullptr)

#define CONCURRENT_CONTROL2(i, a, b, c, l) LFortran::AST::make_ConcurrentControl_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), EXPR(c))


#define CONCURRENT_LOCAL(var_list, l) LFortran::AST::make_ConcurrentLocal_t(p.m_a, l, \
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_LOCAL_INIT(var_list, l) LFortran::AST::make_ConcurrentLocalInit_t(p.m_a, l,\
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_SHARED(var_list, l) LFortran::AST::make_ConcurrentShared_t(p.m_a, l,\
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_DEFAULT(l) LFortran::AST::make_ConcurrentDefault_t(p.m_a, l)

#define CONCURRENT_REDUCE(op, var_list, l) LFortran::AST::make_ConcurrentReduce_t(p.m_a, l, \
        op, \
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define REDUCE_OP_TYPE_ADD(l) LFortran::AST::reduce_opType::ReduceAdd
#define REDUCE_OP_TYPE_MUL(l) LFortran::AST::reduce_opType::ReduceMul
#define REDUCE_OP_TYPE_ID(id, l) convert_id_to_reduce_type(l, id)

#define VAR_DECL(type, attrs, syms, l) make_Declaration_t(p.m_a, l, \
        DECL(p.m_a, syms, type->string.c_str(p.m_a), type->kind, attrs), syms.size())
#define VAR_DECL2(attr, syms, l) make_Declaration_t(p.m_a, l, \
        DECL2(p.m_a, syms, nullptr, attr), syms.size())
#define VAR_DECL3(attr, l) make_Declaration_t(p.m_a, l, \
        DECL2b(p.m_a, attr), 1)
#define VAR_DECL4(id, l) LFortran::AST::make_Declaration_t(p.m_a, l, \
        nullptr, 0)
#define PARAMETER_ITEM(name, value, l) make_parameter_item_t(l, \
        name2char(name), down_cast<LFortran::AST::expr_t>(value))
#define VAR_DECL5(items, l) LFortran::AST::make_ParameterStatement_t(p.m_a, l, \
        items.p, items.size())

#define VAR_SYM_DECL1(id, l)         DECL3(p.m_a, id, nullptr, nullptr)
#define VAR_SYM_DECL2(id, e, l)      DECL3(p.m_a, id, nullptr, EXPR(e))
#define VAR_SYM_DECL3(id, a, l)      DECL3(p.m_a, id, &a, nullptr)
#define VAR_SYM_DECL4(id, a, e, l)   DECL3(p.m_a, id, &a, EXPR(e))
// TODO: Extend AST to express a => b()
#define VAR_SYM_DECL5(id, e, l)      DECL3(p.m_a, id, nullptr, EXPR(e))
// TODO: Extend AST to express a(:) => b()
#define VAR_SYM_DECL6(id, a, e, l)   DECL3(p.m_a, id, &a, EXPR(e))
#define VAR_SYM_DECL7(l)             DECL2c(p.m_a, l)

#define ARRAY_COMP_DECL1(a, l)       DIM1(p.m_a, l, EXPR(INTEGER(1, l)), EXPR(a))
#define ARRAY_COMP_DECL1k(id, a, l)       DIM1k(p.m_a, l, id, EXPR(INTEGER(1, l)), EXPR(a))
#define ARRAY_COMP_DECL2(a, b, l)    DIM1(p.m_a, l, EXPR(a), EXPR(b))
#define ARRAY_COMP_DECL3(a, l)       DIM1(p.m_a, l, EXPR(a), nullptr)
#define ARRAY_COMP_DECL4(b, l)       DIM1(p.m_a, l, nullptr, EXPR(b))
#define ARRAY_COMP_DECL5(l)          DIM1(p.m_a, l, nullptr, nullptr)

#define ARRAY_COMP_DECL1d(a, l)       DIM1d(l, EXPR(INTEGER(1, l)), EXPR(a))
#define ARRAY_COMP_DECL2d(a, b, l)    DIM1d(l, EXPR(a), EXPR(b))
#define ARRAY_COMP_DECL3d(a, l)       DIM1d(l, EXPR(a), nullptr)
#define ARRAY_COMP_DECL4d(b, l)       DIM1d(l, nullptr, EXPR(b))
#define ARRAY_COMP_DECL5d(l)          DIM1d(l, nullptr, nullptr)

#define VARMOD(a, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

#define VARMOD2(a, b, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ ATTR_ARG(p.m_a, l, b), \
        /*n_args*/ 1, \
        nullptr, \
        0)

#define FN_MOD1(a, l) LFortran::AST::make_FnMod_t(p.m_a, l, \
        a->string.c_str(p.m_a))

#define FN_MOD_PURE(l) make_Attribute_t(p.m_a, l, \
        nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

#define FN_MOD_ELEMENTAL(l) make_Attribute_t(p.m_a, l, \
        nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

#define FN_MOD_RECURSIVE(l) make_Attribute_t(p.m_a, l, \
        nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

#define FN_MOD_MODULE(l) make_Attribute_t(p.m_a, l, \
        nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

#define FN_MOD_IMPURE(l) make_Attribute_t(p.m_a, l, \
        nullptr, \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        nullptr, \
        0)

LFortran::Str Str_from_string(Allocator &al, const std::string &s) {
        LFortran::Str r;
        r.from_str(al, s);
        return r;
}

#define VARMOD3(a, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ ATTR_ARG(p.m_a, l, Str_from_string(p.m_a, "inout")), \
        /*n_args*/ 1, \
        nullptr, \
        0)

#define VARMOD_DIM(a, b, l) make_Attribute_t(p.m_a, l, \
        a.c_str(p.m_a), \
        /*args*/ nullptr, \
        /*n_args*/ 0, \
        b.p, \
        b.size())

ast_t* FUNCCALLORARRAY0(Allocator &al, const ast_t *id,
        const LFortran::Vec<LFortran::FnArg> &args, Location &l) {
    LFortran::Vec<LFortran::AST::fnarg_t> v;
    v.reserve(al, args.size());
    LFortran::Vec<LFortran::AST::keyword_t> v2;
    v2.reserve(al, args.size());
    for (auto &item : args) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_FuncCallOrArray_t(al, l,
        /*char* a_func*/ name2char(id),
        /*expr_t** a_args*/ v.p, /*size_t n_args*/ v.size(),
        /*keyword_t* a_keywords*/ v2.p, /*size_t n_keywords*/ v2.size());
}

#define FUNCCALLORARRAY(id, args, l) FUNCCALLORARRAY0(p.m_a, id, args, l)

#define SELECT(cond, body, def, l) make_Select_t(p.m_a, l, \
        EXPR(cond), \
        CASE_STMTS(body), body.size(), \
        STMTS(def), def.size())

#define CASE_STMT(cond, body, l) make_CaseStmt_t(p.m_a, l, \
        EXPRS(cond), cond.size(), STMTS(body), body.size())
#define CASE_STMT2(cond, body, l) make_CaseStmt_t(p.m_a, l, \
        nullptr, 0, STMTS(body), body.size())
#define CASE_STMT3(cond, body, l) make_CaseStmt_t(p.m_a, l, \
        nullptr, 0, STMTS(body), body.size())
#define CASE_STMT4(cond1, cond2, body, l) make_CaseStmt_t(p.m_a, l, \
        nullptr, 0, STMTS(body), body.size())

#define USE1(mod, l) make_Use_t(p.m_a, l, \
        name2char(mod), \
        nullptr, 0)
#define USE2(mod, syms, l) make_Use_t(p.m_a, l, \
        name2char(mod), \
        USE_SYMBOLS(syms), syms.size())

#define USE_SYMBOL1(x, l) make_UseSymbol_t(p.m_a, l, \
        name2char(x), nullptr)
#define USE_SYMBOL2(x, y, l) make_UseSymbol_t(p.m_a, l, \
        name2char(y), name2char(x))
#define USE_SYMBOL3(l) make_UseSymbol_t(p.m_a, l, \
        nullptr, nullptr)

#define MODULE(name, decl, contains, l) make_Module_t(p.m_a, l, \
        name2char(name), \
        /*unit_decl1_t** a_use*/ nullptr, /*size_t n_use*/ 0, \
        /*unit_decl2_t** a_decl*/ nullptr, /*size_t n_decl*/ 0, \
        /*program_unit_t** a_contains*/ CONTAINS(contains), /*size_t n_contains*/ contains.size())
#define PRIVATE0(l) make_Private_t(p.m_a, l, \
        nullptr, 0)
#define PRIVATE(syms, l) make_Private_t(p.m_a, l, \
        nullptr, 0)
#define PUBLIC(syms, l) make_Public_t(p.m_a, l, \
        nullptr, 0)
#define INTERFACE(name, l) make_Interface_t(p.m_a, l, \
        name2char(name), nullptr, 0)
#define INTERFACE3(l) make_Interface_t(p.m_a, l, \
        nullptr, nullptr, 0)

#define INTERFACE2(contains, l) LFortran::AST::make_Interface2_t(p.m_a, l, \
        nullptr, nullptr, 0)

// TODO: Add DerivedType AST node
#define DERIVED_TYPE(name, l) make_Interface_t(p.m_a, l, \
        name2char(name), nullptr, 0)

#endif
