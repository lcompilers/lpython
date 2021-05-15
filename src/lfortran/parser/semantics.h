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

// This is only used in parser.tab.cc, nowhere else, so we simply include
// everything from LFortran::AST to save typing:
using namespace LFortran::AST;
using LFortran::Location;
using LFortran::Vec;
using LFortran::FnArg;
using LFortran::VarType;
using LFortran::SemanticError;
using LFortran::ArgStarKw;


static inline expr_t* EXPR(const ast_t *f)
{
    return down_cast<expr_t>(f);
}

static inline expr_t* EXPR_OPT(const ast_t *f)
{
    if (f) {
        return EXPR(f);
    } else {
        return nullptr;
    }
}

static inline bind_t* bind_opt(const ast_t *f)
{
    if (f) {
        return down_cast<bind_t>(f);
    } else {
        return nullptr;
    }
}

static inline char* name2char(const ast_t *n)
{
    return down_cast2<Name_t>(n)->m_id;
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
#define DECLS(x) VEC_CAST(x, unit_decl2)
#define USES(x) VEC_CAST(x, unit_decl1)
#define STMTS(x) VEC_CAST(x, stmt)
#define CONTAINS(x) VEC_CAST(x, program_unit)
#define ATTRS(x) VEC_CAST(x, attribute)
#define EXPRS(x) VEC_CAST(x, expr)
#define CASE_STMTS(x) VEC_CAST(x, case_stmt)
#define TYPE_STMTS(x) VEC_CAST(x, type_stmt)
#define USE_SYMBOLS(x) VEC_CAST(x, use_symbol)
#define CONCURRENT_CONTROLS(x) VEC_CAST(x, concurrent_control)
#define CONCURRENT_LOCALITIES(x) VEC_CAST(x, concurrent_locality)
#define INTERFACE_ITEMS(x) VEC_CAST(x, interface_item)

Vec<ast_t*> A2LIST(Allocator &al, ast_t *x) {
    Vec<ast_t*> v;
    v.reserve(al, 1);
    v.push_back(al, x);
    return v;
}

static inline stmt_t** IFSTMTS(Allocator &al, ast_t* x)
{
    stmt_t **s = al.allocate<stmt_t*>();
    *s = down_cast<stmt_t>(x);
    return s;
}

static inline kind_item_t *make_kind_item_t(Allocator &al,
    Location &loc, char *id, ast_t *value, kind_item_typeType type)
{
    kind_item_t *r = al.allocate<kind_item_t>(1);
    r->loc = loc;
    r->m_id = id;
    if (value) {
        r->m_value = down_cast<expr_t>(value);
    } else {
        r->m_value = nullptr;
    }
    r->m_type = type;
    return r;
}

static inline Vec<kind_item_t> a2kind_list(Allocator &al,
    Location &loc, ast_t *value)
{
    kind_item_t r;
    r.loc = loc;
    r.m_id = nullptr;
    r.m_value = down_cast<expr_t>(value);
    r.m_type = kind_item_typeType::Value;
    Vec<kind_item_t> v;
    v.reserve(al, 1);
    v.push_back(al, r);
    return v;
}

#define KIND_ARG1(k, l) make_kind_item_t(p.m_a, l, nullptr, k, \
        kind_item_typeType::Value)
#define KIND_ARG1S(l) make_kind_item_t(p.m_a, l, nullptr, nullptr, \
        kind_item_typeType::Star)
#define KIND_ARG1C(l) make_kind_item_t(p.m_a, l, nullptr, nullptr, \
        kind_item_typeType::Colon)
#define KIND_ARG2(id, k, l) make_kind_item_t(p.m_a, l, name2char(id), k, \
        kind_item_typeType::Value)
#define KIND_ARG2S(id, l) make_kind_item_t(p.m_a, l, name2char(id), nullptr, \
        kind_item_typeType::Star)
#define KIND_ARG2C(id, l) make_kind_item_t(p.m_a, l, name2char(id), nullptr, \
        kind_item_typeType::Colon)

#define SIMPLE_ATTR(x, l) make_SimpleAttribute_t( \
            p.m_a, l, simple_attributeType::Attr##x)
#define INTENT(x, l) make_AttrIntent_t( \
            p.m_a, l, attr_intentType::x)
#define BIND(x, l) make_AttrBind_t( \
            p.m_a, l, name2char(x))
#define EXTENDS(x, l) make_AttrExtends_t( \
            p.m_a, l, name2char(x))
#define DIMENSION(x, l) make_AttrDimension_t( \
            p.m_a, l, \
            x.p, x.size())
#define DIMENSION0(l) make_AttrDimension_t( \
            p.m_a, l, \
            nullptr, 0)

#define ATTR_TYPE(x, l) make_AttrType_t( \
            p.m_a, l, \
            decl_typeType::Type##x, \
            nullptr, 0, \
            nullptr)

#define ATTR_TYPE_INT(x, n, l) make_AttrType_t( \
            p.m_a, l, \
            decl_typeType::Type##x, \
            a2kind_list(p.m_a, l, INTEGER(n, l)).p, 1, \
            nullptr)

#define ATTR_TYPE_KIND(x, kind, l) make_AttrType_t( \
            p.m_a, l, \
            decl_typeType::Type##x, \
            kind.p, kind.size(), \
            nullptr)

#define ATTR_TYPE_NAME(x, name, l) make_AttrType_t( \
            p.m_a, l, \
            decl_typeType::Type##x, \
            nullptr, 0, \
            name2char(name))

#define IMPORT0(x, l) make_Import_t( \
            p.m_a, l, \
            nullptr, 0, \
            import_modifierType::Import##x)
#define IMPORT1(args, x, l) make_Import_t( \
            p.m_a, l, \
            REDUCE_ARGS(p.m_a, args), args.size(), \
            import_modifierType::Import##x)


#define VAR_DECL1(vartype, xattr, varsym, l) \
        make_Declaration_t(p.m_a, l, \
        down_cast<decl_attribute_t>(vartype), \
        VEC_CAST(xattr, decl_attribute), xattr.n, \
        varsym.p, varsym.n)

decl_attribute_t** VAR_DECL2b(Allocator &al,
            ast_t *xattr0) {
    decl_attribute_t** a = al.allocate<decl_attribute_t*>(1);
    *a = down_cast<decl_attribute_t>(xattr0);
    return a;
}

decl_attribute_t** VAR_DECL_NAMELISTb(Allocator &al,
        Location &loc,
            char *name) {
    Vec<decl_attribute_t*> v;
    v.reserve(al, 1);
    ast_t* a = make_AttrNamelist_t(al, loc, name);
    v.push_back(al, down_cast<decl_attribute_t>(a));
    return v.p;
}

var_sym_t* VAR_DECL_NAMELISTc(Allocator &al,
            Vec<ast_t*> id_list) {
    var_sym_t* a = al.allocate<var_sym_t>(id_list.size());
    for (size_t i=0; i<id_list.size(); i++) {
        a[i].m_name = name2char(id_list[i]);
    }
    return a;
}

decl_attribute_t** VAR_DECL_PARAMETERb(Allocator &al,
        Location &loc) {
    Vec<decl_attribute_t*> v;
    v.reserve(al, 1);
    ast_t* a = make_SimpleAttribute_t(al, loc,
            simple_attributeType::AttrParameter);
    v.push_back(al, down_cast<decl_attribute_t>(a));
    return v.p;
}

#define VAR_DECL2(xattr0, l) \
        make_Declaration_t(p.m_a, l, \
        nullptr, \
        VAR_DECL2b(p.m_a, xattr0), 1, \
        nullptr, 0)

#define VAR_DECL3(xattr0, varsym, l) \
        make_Declaration_t(p.m_a, l, \
        nullptr, \
        VAR_DECL2b(p.m_a, xattr0), 1, \
        varsym.p, varsym.n)

#define VAR_DECL_NAMELIST(id, id_list, l) \
        make_Declaration_t(p.m_a, l, \
        nullptr, \
        VAR_DECL_NAMELISTb(p.m_a, l, name2char(id)), 1, \
        VAR_DECL_NAMELISTc(p.m_a, id_list), id_list.n)

#define VAR_DECL_PARAMETER(varsym, l) \
        make_Declaration_t(p.m_a, l, \
        nullptr, \
        VAR_DECL_PARAMETERb(p.m_a, l), 1, \
        varsym.p, varsym.n)

#define ENUM(l) make_Enum_t(p.m_a, l)

#define IMPLICIT_NONE(l) make_ImplicitNone_t(p.m_a, l, \
        nullptr, 0)
#define IMPLICIT_NONE2(x, l) make_ImplicitNone_t(p.m_a, l, \
        VEC_CAST(x, implicit_none_spec), x.size())
#define IMPLICIT_NONE_EXTERNAL(l) make_ImplicitNoneExternal_t(p.m_a, l, 0)
#define IMPLICIT_NONE_TYPE(l) make_ImplicitNoneType_t(p.m_a, l)

#define IMPLICIT(t, spec, l) make_Implicit_t(p.m_a, l, \
        down_cast<decl_attribute_t>(t), \
        VEC_CAST(spec, letter_spec), spec.size())

#define LETTER_SPEC1(a, l) make_LetterSpec_t(p.m_a, l, \
        nullptr, name2char(a))
#define LETTER_SPEC2(a, b, l) make_LetterSpec_t(p.m_a, l, \
        name2char(a), name2char(b))

#define VAR_SYM(xout, xname, xdimp, xdimn, xinit, xloc) \
            xout = p.m_a.allocate<var_sym_t>(1); \
            xout->loc = xloc; \
            xout->m_name = name2char(xname); \
            xout->m_dim = xdimp; \
            xout->n_dim = xdimn; \
            xout->m_initializer=down_cast<expr_t>(xinit);

#define VAR_SYM2(xout, xname, xdimp, xdimn, xloc) \
            xout = p.m_a.allocate<var_sym_t>(1); \
            xout->loc = xloc; \
            xout->m_name = name2char(xname); \
            xout->m_dim = xdimp; \
            xout->n_dim = xdimn; \
            xout->m_initializer=nullptr;


static inline expr_t** DIMS2EXPRS(Allocator &al, const Vec<FnArg> &d)
{
    if (d.size() == 0) {
        return nullptr;
    } else {
        expr_t **s = al.allocate<expr_t*>(d.size());
        for (size_t i=0; i < d.size(); i++) {
            // TODO: we need to change this to allow both array and fn arguments
            // Right now we assume everything is a function argument
            if (d[i].keyword) {
                if (d[i].kw.m_value) {
                    s[i] = d[i].kw.m_value;
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

static inline Vec<kind_item_t> empty()
{
    Vec<kind_item_t> r;
    r.from_pointer_n(nullptr, 0);
    return r;
}

static inline Vec<ast_t*> empty_vecast()
{
    Vec<ast_t*> r;
    r.from_pointer_n(nullptr, 0);
    return r;
}

static inline Vec<struct_member_t> empty5()
{
    Vec<struct_member_t> r;
    r.from_pointer_n(nullptr, 0);
    return r;
}

static inline VarType* VARTYPE0_(Allocator &al,
        const LFortran::Str &s, const Vec<kind_item_t> kind, Location &l)
{
    VarType *r = al.allocate<VarType>(1);
    r->loc = l;
    r->string = s;
    r->identifier = nullptr;
    r->kind = kind;
    return r;
}

static inline VarType* VARTYPE4_(Allocator &al,
        const LFortran::Str &s, const ast_t *id, Location &l)
{
    VarType *r = al.allocate<VarType>(1);
    r->loc = l;
    r->string = s;
    char *derived_type_name = name2char(id);
    r->identifier = derived_type_name;
    Vec<kind_item_t> kind;
    kind.reserve(al, 1);
    r->kind = kind;
    return r;
}

#define VARTYPE0(s, l) VARTYPE0_(p.m_a, s, empty(), l)
#define VARTYPE3(s, k, l) VARTYPE0_(p.m_a, s, k, l)
#define VARTYPE4(s, k, l) VARTYPE4_(p.m_a, s, k, l)

static inline FnArg* DIM1(Allocator &al, Location &l,
    expr_t *a, expr_t *b, expr_t *c)
{
    FnArg *s = al.allocate<FnArg>();
    s->keyword = false;
    s->arg.loc = l;
    s->arg.m_start = a;
    s->arg.m_end = b;
    s->arg.m_step = c;
    return s;
}

static inline FnArg* DIM1k(Allocator &al, Location &l,
        ast_t *id, expr_t */*a*/, expr_t *b)
{
    FnArg *s = al.allocate<FnArg>();
    s->keyword = true;
    s->kw.loc = l;
    s->kw.m_arg = name2char(id);
    s->kw.m_value = b;
    return s;
}

static inline dimension_t* DIM1d(Allocator &al, Location &l, expr_t *a, expr_t *b)
{
    dimension_t *s = al.allocate<dimension_t>();
    s->loc = l;
    s->m_start = a;
    s->m_end = b;
    s->m_end_star = dimension_typeType::DimensionExpr;
    return s;
}

static inline dimension_t* DIM1d_star(Allocator &al, Location &l, expr_t *a)
{
    dimension_t *s = al.allocate<dimension_t>();
    s->loc = l;
    s->m_start = a;
    s->m_end = nullptr;
    s->m_end_star = dimension_typeType::DimensionStar;
    return s;
}


static inline arg_t* ARGS(Allocator &al, Location &l,
    const Vec<ast_t*> args)
{
    arg_t *a = al.allocate<arg_t>(args.size());
    for (size_t i=0; i < args.size(); i++) {
        a[i].loc = l;
        a[i].m_arg = name2char(args.p[i]);
    }
    return a;
}

static inline char** REDUCE_ARGS(Allocator &al, const Vec<ast_t*> args)
{
    char **a = al.allocate<char*>(args.size());
    for (size_t i=0; i < args.size(); i++) {
        a[i] = name2char(args.p[i]);
    }
    return a;
}

static inline reduce_opType convert_id_to_reduce_type(
        const Location &loc, const ast_t *id)
{
        std::string s_id = down_cast2<Name_t>(id)->m_id;
        if (s_id == "MIN" ) {
                return reduce_opType::ReduceMIN;
        } else if (s_id == "MAX") {
                return reduce_opType::ReduceMAX;
        } else {
                throw SemanticError("Unsupported operation in reduction", loc);
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
#define TRUE(l) make_Logical_t(p.m_a, l, true)
#define FALSE(l) make_Logical_t(p.m_a, l, false)

#define STRCONCAT(x, y, l) make_StrOp_t(p.m_a, l, EXPR(x), stroperatorType::Concat, EXPR(y))

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

ast_t* implied_do_loop(Allocator &al, Location &loc,
        Vec<ast_t*> &ex_list,
        ast_t* i,
        ast_t* low,
        ast_t* high) {
    return make_ImpliedDoLoop_t(al, loc,
            EXPRS(ex_list), ex_list.size(),
            name2char(i),
            EXPR(low),
            EXPR(high),
            nullptr);
}

ast_t* implied_do1(Allocator &al, Location &loc,
        ast_t* ex,
        ast_t* i,
        ast_t* low,
        ast_t* high) {
    Vec<ast_t*> v;
    v.reserve(al, 1);
    v.push_back(al, ex);
    return implied_do_loop(al, loc, v, i, low, high);
}

ast_t* implied_do2(Allocator &al, Location &loc,
        ast_t* ex1,
        ast_t* ex2,
        ast_t* i,
        ast_t* low,
        ast_t* high) {
    Vec<ast_t*> v;
    v.reserve(al, 2);
    v.push_back(al, ex1);
    v.push_back(al, ex2);
    return implied_do_loop(al, loc, v, i, low, high);
}

ast_t* implied_do3(Allocator &al, Location &loc,
        ast_t* ex1,
        ast_t* ex2,
        Vec<ast_t*> ex_list,
        ast_t* i,
        ast_t* low,
        ast_t* high) {
    Vec<ast_t*> v;
    v.reserve(al, 2+ex_list.size());
    v.push_back(al, ex1);
    v.push_back(al, ex2);
    for (size_t i=0; i<ex_list.size(); i++) {
        v.push_back(al, ex_list[i]);
    }
    return implied_do_loop(al, loc, v, i, low, high);
}

#define IMPLIED_DO_LOOP1(ex, i, low, high, l) \
    implied_do1(p.m_a, l, ex, i, low, high)
#define IMPLIED_DO_LOOP2(ex1, ex2, i, low, high, l) \
    implied_do2(p.m_a, l, ex1, ex2, i, low, high)
#define IMPLIED_DO_LOOP3(ex1, ex2, ex_list, i, low, high, l) \
    implied_do3(p.m_a, l, ex1, ex2, ex_list, i, low, high)

#define SYMBOL(x, l) make_Name_t(p.m_a, l, x.c_str(p.m_a), nullptr, 0)
#define INTEGER(x, l) make_Num_t(p.m_a, l, x)
#define REAL(x, l) make_Real_t(p.m_a, l, x.c_str(p.m_a))
#define COMPLEX(x, y, l) make_Complex_t(p.m_a, l, EXPR(x), EXPR(y))
#define STRING(x, l) make_Str_t(p.m_a, l, x.c_str(p.m_a))
#define ASSIGNMENT(x, y, l) make_Assignment_t(p.m_a, l, 0, EXPR(x), EXPR(y))
#define ASSOCIATE(x, y, l) make_Associate_t(p.m_a, l, 0, EXPR(x), EXPR(y))
#define GOTO(x, l) make_GoTo_t(p.m_a, l, 0, x)


ast_t* SUBROUTINE_CALL0(Allocator &al, const ast_t *id,
        const Vec<FnArg> &args, Location &l) {
    Vec<fnarg_t> v;
    v.reserve(al, args.size());
    Vec<keyword_t> v2;
    v2.reserve(al, args.size());
    for (auto &item : args) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_SubroutineCall_t(al, l, 0,
        /*char* a_func*/ name2char(id),
        /*expr_t** a_args*/ v.p, /*size_t n_args*/ v.size(),
        /*keyword_t* a_keywords*/ v2.p, /*size_t n_keywords*/ v2.size());
}
#define SUBROUTINE_CALL(name, args, l) SUBROUTINE_CALL0(p.m_a, \
        name, args, l)
#define SUBROUTINE_CALL2(name, l) make_SubroutineCall_t(p.m_a, l, 0, \
        name2char(name), \
        nullptr, 0, nullptr, 0)

Vec<fnarg_t> FNARGS(Allocator &al,
        const Vec<FnArg> &args) {
    Vec<fnarg_t> v;
    v.reserve(al, args.size());
    Vec<keyword_t> v2;
    v2.reserve(al, args.size());
    for (auto &item : args) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
            LFORTRAN_ASSERT(false);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return v;
}
ast_t* ALLOCATE_STMT0(Allocator &al,
        const Vec<FnArg> &args, Location &l) {
    Vec<fnarg_t> v;
    v.reserve(al, args.size());
    Vec<keyword_t> v2;
    v2.reserve(al, args.size());
    for (auto &item : args) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_Allocate_t(al, l, 0,
        /*expr_t** a_args*/ v.p, /*size_t n_args*/ v.size(),
        /*keyword_t* a_keywords*/ v2.p, /*size_t n_keywords*/ v2.size());
}
#define ALLOCATE_STMT(args, l) ALLOCATE_STMT0(p.m_a, args, l)
#define DEALLOCATE_STMT(args, l) make_Deallocate_t(p.m_a, l, 0, \
        FNARGS(p.m_a, args).p, args.size())

char* print_format_to_str(Allocator &al, const std::string &fmt) {
    LFORTRAN_ASSERT(fmt[0] == '(');
    LFORTRAN_ASSERT(fmt[fmt.size()-1] == ')');
    std::string fmt2 = fmt.substr(1, fmt.size()-2);
    LFortran::Str s;
    s.from_str_view(fmt2);
    return s.c_str(al);
}

#define PRINT0(l) make_Print_t(p.m_a, l, 0, nullptr, nullptr, 0)
#define PRINT(args, l) make_Print_t(p.m_a, l, 0, nullptr, EXPRS(args), args.size())
#define PRINTF0(fmt, l) make_Print_t(p.m_a, l, 0, \
        print_format_to_str(p.m_a, fmt.str()), nullptr, 0)
#define PRINTF(fmt, args, l) make_Print_t(p.m_a, l, 0, \
        print_format_to_str(p.m_a, fmt.str()), EXPRS(args), args.size())

ast_t* WRITE1(Allocator &al,
        const Vec<ArgStarKw> &args0,
        const Vec<ast_t*> &args,
        Location &l) {
    Vec<argstar_t> v;
    v.reserve(al, args0.size());
    Vec<kw_argstar_t> v2;
    v2.reserve(al, args0.size());
    for (auto &item : args0) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_Write_t(al, l, 0,
        v.p, v.size(),
        v2.p, v2.size(),
        EXPRS(args), args.size());
}

ast_t* READ1(Allocator &al,
        const Vec<ArgStarKw> &args0,
        const Vec<ast_t*> &args,
        Location &l) {
    Vec<argstar_t> v;
    v.reserve(al, args0.size());
    Vec<kw_argstar_t> v2;
    v2.reserve(al, args0.size());
    for (auto &item : args0) {
        if (item.keyword) {
            v2.push_back(al, item.kw);
        } else {
            v.push_back(al, item.arg);
        }
    }
    return make_Read_t(al, l, 0,
        v.p, v.size(),
        v2.p, v2.size(),
        EXPRS(args), args.size());
}

void extract_args1(Allocator &al,
        Vec<expr_t*> &v,
        Vec<keyword_t> &v2,
        const Vec<ArgStarKw> &args0) {
    v.reserve(al, args0.size());
    v2.reserve(al, args0.size());
    for (auto &item : args0) {
        if (item.keyword) {
            keyword_t kw;
            LFORTRAN_ASSERT(item.kw.m_value != nullptr);
            kw.loc = item.kw.loc;
            kw.m_value = item.kw.m_value;
            kw.m_arg = item.kw.m_arg;
            v2.push_back(al, kw);
        } else {
            LFORTRAN_ASSERT(item.arg.m_value != nullptr);
            v.push_back(al, item.arg.m_value);
        }
    }
}

template <typename ASTConstructor>
ast_t* builtin1(Allocator &al,
        const Vec<ArgStarKw> &args0,
        Location &l, ASTConstructor cons) {
    Vec<expr_t*> v;
    Vec<keyword_t> v2;
    extract_args1(al, v, v2, args0);
    return cons(al, l, 0,
        v.p, v.size(),
        v2.p, v2.size());
}

template <typename ASTConstructor>
ast_t* builtin2(Allocator &al,
        const Vec<ArgStarKw> &args0,
        const Vec<ast_t*> &ex_list,
        Location &l, ASTConstructor cons) {
    Vec<expr_t*> v;
    Vec<keyword_t> v2;
    extract_args1(al, v, v2, args0);
    return cons(al, l, 0,
        v.p, v.size(),
        v2.p, v2.size(), EXPRS(ex_list), ex_list.size());
}

template <typename ASTConstructor>
ast_t* builtin3(Allocator &al,
        const Vec<ArgStarKw> &args0,
        Location &l, ASTConstructor cons) {
    Vec<expr_t*> v;
    Vec<keyword_t> v2;
    extract_args1(al, v, v2, args0);
    return cons(al, l,
        v.p, v.size(),
        v2.p, v2.size());
}

#define WRITE_ARG1(out, arg0) \
        out = p.m_a.make_new<ArgStarKw>(); \
        out->keyword = false; \
        if (arg0 == nullptr) { \
            out->arg.m_value = nullptr; \
        } else { \
            out->arg.m_value = down_cast< \
                    expr_t>(arg0); \
        }

#define WRITE_ARG2(out, id0, arg0) \
        out = p.m_a.make_new<ArgStarKw>(); \
        out->keyword = true; \
        out->kw.m_arg = name2char(id0); \
        if (arg0 == nullptr) { \
            out->kw.m_value = nullptr; \
        } else { \
            out->kw.m_value = down_cast<expr_t>(arg0); \
        }


#define WRITE0(args0, l) WRITE1(p.m_a, args0, empty_vecast(), l)
#define WRITE(args0, args, l) WRITE1(p.m_a, args0, args, l)

#define READ0(args0, l) READ1(p.m_a, args0, empty_vecast(), l)
#define READ(args0, args, l) READ1(p.m_a, args0, args, l)

#define OPEN(args0, l) builtin1(p.m_a, args0, l, make_Open_t)
#define CLOSE(args0, l) builtin1(p.m_a, args0, l, make_Close_t)
#define REWIND(args0, l) builtin1(p.m_a, args0, l, make_Rewind_t)
#define NULLIFY(args0, l) builtin1(p.m_a, args0, l, make_Nullify_t)
#define BACKSPACE(args0, l) builtin1(p.m_a, args0, l, make_Backspace_t)

#define INQUIRE0(args0, l) builtin2(p.m_a, args0, empty_vecast(), l, \
            make_Inquire_t)
#define INQUIRE(args0, args, l) builtin2(p.m_a, args0, args, l, make_Inquire_t)
#define REWIND2(arg, l) make_Rewind_t(p.m_a, l, 0, \
            EXPRS(A2LIST(p.m_a, arg)), 1, nullptr, 0)
#define REWIND3(arg, l) make_Rewind_t(p.m_a, l, 0, \
            EXPRS(A2LIST(p.m_a, INTEGER(arg, l))), 1, nullptr, 0)

#define BIND2(args0, l) builtin3(p.m_a, args0, l, make_Bind_t)


void CONVERT_FNARRAYARG_FNARG(Allocator &al,
        struct_member_t &s,
        const Vec<FnArg> &args)
{
    Vec<fnarg_t> v;
    v.reserve(al, args.size());
    for (auto &item : args) {
        LFORTRAN_ASSERT(!item.keyword);
        v.push_back(al, item.arg);
    }
    s.m_args = v.p;
    s.n_args = v.size();
}

#define STRUCT_MEMBER1(out, id) \
            out = p.m_a.make_new<struct_member_t>(); \
            out->m_name = name2char(id); \
            out->m_args = nullptr; \
            out->n_args = 0;

#define STRUCT_MEMBER2(out, id, member) \
            out = p.m_a.make_new<struct_member_t>(); \
            out->m_name = name2char(id); \
            CONVERT_FNARRAYARG_FNARG(p.m_a, *out, member);

#define NAME1(out, id, member, l) \
            out = make_Name_t(p.m_a, l, \
                name2char(id), \
                member.p, member.n);

// Converts (line, col) to a linear position.
uint64_t linecol_to_pos(const std::string &s, uint16_t line, uint16_t col) {
    uint64_t pos = 0;
    uint64_t l = 1;
    while (true) {
        if (l == line) break;
        if (pos >= s.size()) return 0;
        while (s[pos] != '\n' && pos < s.size()) pos++;
        l++;
        pos++;
    }
    pos += col-1;
    if (pos >= s.size()) return 0;
    return pos;
}

char* format_to_str(Allocator &al, Location &loc, const std::string &inp) {
    uint64_t first = linecol_to_pos(inp, loc.first_line, loc.first_column);
    uint64_t last = linecol_to_pos(inp, loc.last_line, loc.last_column);
    std::string fmt = inp.substr(first, last-first+1);
    if (fmt[fmt.size()-1] != ')') {
        // This is a workaround for a bug that the last_column is too small
        // for multiline comments
        std::size_t found = inp.find(")\n", last);
        LFORTRAN_ASSERT(found != std::string::npos);
        last = found;
        fmt = inp.substr(first, last-first+1);
    }
    LFORTRAN_ASSERT(fmt[fmt.size()-1] == ')');
    std::size_t found = fmt.find('(');
    LFORTRAN_ASSERT(found != std::string::npos);
    fmt = fmt.substr(found+1, fmt.size()-found-2);

    LFortran::Str s;
    s.from_str_view(fmt);
    return s.c_str(al);
}

#define FORMAT(l) make_Format_t(p.m_a, l, 0, \
        format_to_str(p.m_a, l, p.inp))

#define STOP(l) make_Stop_t(p.m_a, l, 0, nullptr)
#define STOP1(e, l) make_Stop_t(p.m_a, l, 0, EXPR(e))
#define ERROR_STOP(l) make_ErrorStop_t(p.m_a, l, 0, nullptr)
#define ERROR_STOP1(e, l) make_ErrorStop_t(p.m_a, l, 0, EXPR(e))

#define EXIT(l) make_Exit_t(p.m_a, l, 0, nullptr)
#define EXIT2(id, l) make_Exit_t(p.m_a, l, 0, name2char(id))
#define RETURN(l) make_Return_t(p.m_a, l, 0)
#define CYCLE(l) make_Cycle_t(p.m_a, l, 0, nullptr)
#define CYCLE2(id, l) make_Cycle_t(p.m_a, l, 0, name2char(id))
#define CONTINUE(l) make_Continue_t(p.m_a, l, 0)
#define SUBROUTINE(name, args, bind, use, import, implicit, decl, stmts, contains, l) \
    make_Subroutine_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*bind*/ bind_opt(bind), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*m_import*/ VEC_CAST(import, import_statement), \
        /*n_import*/ import.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())
#define PROCEDURE(name, args, use, import, implicit, decl, stmts, contains, l) \
    make_Procedure_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*m_import*/ VEC_CAST(import, import_statement), \
        /*n_import*/ import.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())

char *str_or_null(Allocator &al, const LFortran::Str &s) {
    if (s.size() == 0) {
        return nullptr;
    } else {
        return s.c_str(al);
    }
}

#define FUNCTION(fn_type, name, args, return_var, bind, use, import, implicit, decl, stmts, contains, l) make_Function_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*m_attributes*/ VEC_CAST(fn_type, decl_attribute), \
        /*n_attributes*/ fn_type.size(), \
        /*return_var*/ EXPR_OPT(return_var), \
        /*bind*/ bind_opt(bind), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*m_import*/ VEC_CAST(import, import_statement), \
        /*n_import*/ import.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())
#define FUNCTION0(name, args, return_var, bind, use, import, implicit, decl, stmts, contains, l) make_Function_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*args*/ ARGS(p.m_a, l, args), \
        /*n_args*/ args.size(), \
        /*return_type*/ nullptr, \
        /*return_type*/ 0, \
        /*return_var*/ EXPR_OPT(return_var), \
        /*bind*/ bind_opt(bind), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*m_import*/ VEC_CAST(import, import_statement), \
        /*n_import*/ import.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())
#define PROGRAM(name, use, implicit, decl, stmts, contains, l) make_Program_t(p.m_a, l, \
        /*name*/ name2char(name), \
        /*use*/ USES(use), \
        /*n_use*/ use.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*decl*/ DECLS(decl), \
        /*n_decl*/ decl.size(), \
        /*body*/ STMTS(stmts), \
        /*n_body*/ stmts.size(), \
        /*contains*/ CONTAINS(contains), \
        /*n_contains*/ contains.size())
#define RESULT(x) p.result.push_back(p.m_a, x)

#define STMT_NAME(id_first, id_last, stmt) \
        stmt; \
        ((If_t*)stmt)->m_stmt_name = name2char(id_first); \
        if (std::string(name2char(id_first)) != \
                std::string(name2char(id_last))) { \
            throw LFortran::LFortranException("statement name is inconsistent"); \
        }

#define LABEL(stmt, label) ((Print_t*)stmt)->m_label = label

#define BLOCK(decl, body, l) make_Block_t(p.m_a, l, 0, nullptr, \
        DECLS(decl), decl.size(), \
        STMTS(body), body.size())

#define ASSOCIATE_BLOCK(syms, body, l) make_AssociateBlock_t(p.m_a, l, 0, \
        nullptr, \
        syms.p, syms.size(), \
        STMTS(body), body.size())

#define IFSINGLE(cond, body, l) make_If_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ IFSTMTS(p.m_a, body), \
        /*n_body*/ 1, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF1(cond, body, l) make_If_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF2(cond, body, orelse, l) make_If_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ STMTS(orelse), \
        /*n_orelse*/ orelse.size())

#define IF3(cond, body, ifblock, l) make_If_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ IFSTMTS(p.m_a, ifblock), \
        /*n_orelse*/ 1)

#define WHERESINGLE(cond, body, l) make_Where_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ IFSTMTS(p.m_a, body), \
        /*n_body*/ 1, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define WHERE1(cond, body, l) make_Where_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define WHERE2(cond, body, orelse, l) make_Where_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ STMTS(orelse), \
        /*n_orelse*/ orelse.size())

#define WHERE3(cond, body, whereblock, l) make_Where_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size(), \
        /*a_orelse*/ IFSTMTS(p.m_a, whereblock), \
        /*n_orelse*/ 1)

#define LIST_NEW(l) l.reserve(p.m_a, 4)
#define LIST_ADD(l, x) l.push_back(p.m_a, x)
#define PLIST_ADD(l, x) l.push_back(p.m_a, *x)

#define WHILE(cond, body, l) make_WhileLoop_t(p.m_a, l, 0, nullptr, \
        /*test*/ EXPR(cond), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO1(body, l) make_DoLoop_t(p.m_a, l, 0, nullptr, \
        nullptr, nullptr, nullptr, nullptr, \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO2(i, a, b, body, l) make_DoLoop_t(p.m_a, l, 0, nullptr, \
        name2char(i), EXPR(a), EXPR(b), nullptr, \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO3(i, a, b, c, body, l) make_DoLoop_t(p.m_a, l, 0, nullptr, \
        name2char(i), EXPR(a), EXPR(b), EXPR(c), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define DO_CONCURRENT1(h, loc, body, l) make_DoConcurrentLoop_t(p.m_a, l, 0, nullptr, \
        CONCURRENT_CONTROLS(h), h.size(), \
        nullptr, \
        CONCURRENT_LOCALITIES(loc), loc.size(), \
        STMTS(body), body.size())

#define DO_CONCURRENT2(h, m, loc, body, l) make_DoConcurrentLoop_t(p.m_a, l, 0, nullptr, \
        CONCURRENT_CONTROLS(h), h.size(), \
        EXPR(m), \
        CONCURRENT_LOCALITIES(loc), loc.size(), \
        STMTS(body), body.size())


#define DO_CONCURRENT_REDUCE(i, a, b, reduce, body, l) make_DoConcurrentLoop_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), nullptr, \
        /*reduce*/ REDUCE_TYPE(reduce), \
        /*body*/ STMTS(body), \
        /*n_body*/ body.size())

#define CONCURRENT_CONTROL1(i, a, b, l) make_ConcurrentControl_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), nullptr)

#define CONCURRENT_CONTROL2(i, a, b, c, l) make_ConcurrentControl_t(p.m_a, l, \
        name2char(i), EXPR(a), EXPR(b), EXPR(c))


#define CONCURRENT_LOCAL(var_list, l) make_ConcurrentLocal_t(p.m_a, l, \
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_LOCAL_INIT(var_list, l) make_ConcurrentLocalInit_t(p.m_a, l,\
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_SHARED(var_list, l) make_ConcurrentShared_t(p.m_a, l,\
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define CONCURRENT_DEFAULT(l) make_ConcurrentDefault_t(p.m_a, l)

#define CONCURRENT_REDUCE(op, var_list, l) make_ConcurrentReduce_t(p.m_a, l, \
        op, \
        REDUCE_ARGS(p.m_a, var_list), var_list.size())

#define REDUCE_OP_TYPE_ADD(l) reduce_opType::ReduceAdd
#define REDUCE_OP_TYPE_MUL(l) reduce_opType::ReduceMul
#define REDUCE_OP_TYPE_ID(id, l) convert_id_to_reduce_type(l, id)

#define VAR_SYM_DECL1(id, l)         DECL3(p.m_a, id, nullptr, nullptr)
#define VAR_SYM_DECL2(id, e, l)      DECL3(p.m_a, id, nullptr, EXPR(e))
#define VAR_SYM_DECL3(id, a, l)      DECL3(p.m_a, id, &a, nullptr)
#define VAR_SYM_DECL4(id, a, e, l)   DECL3(p.m_a, id, &a, EXPR(e))
// TODO: Extend AST to express a => b()
#define VAR_SYM_DECL5(id, e, l)      DECL3(p.m_a, id, nullptr, EXPR(e))
// TODO: Extend AST to express a(:) => b()
#define VAR_SYM_DECL6(id, a, e, l)   DECL3(p.m_a, id, &a, EXPR(e))
#define VAR_SYM_DECL7(l)             DECL2c(p.m_a, l)

#define ARRAY_COMP_DECL_0i0(a,l)     DIM1(p.m_a, l, nullptr, EXPR(a), nullptr)
#define ARRAY_COMP_DECL_001(l)       DIM1(p.m_a, l, nullptr, nullptr, EXPR(INTEGER(1,l)))
#define ARRAY_COMP_DECL_a01(a,l)     DIM1(p.m_a, l, EXPR(a), nullptr, EXPR(INTEGER(1,l)))
#define ARRAY_COMP_DECL_0b1(b,l)     DIM1(p.m_a, l, nullptr, EXPR(b), EXPR(INTEGER(1,l)))
#define ARRAY_COMP_DECL_ab1(a,b,l)   DIM1(p.m_a, l, EXPR(a), EXPR(b), EXPR(INTEGER(1,l)))
#define ARRAY_COMP_DECL_00c(c,l)     DIM1(p.m_a, l, nullptr, nullptr, EXPR(c))
#define ARRAY_COMP_DECL_a0c(a,c,l)   DIM1(p.m_a, l, EXPR(a), nullptr, EXPR(c))
#define ARRAY_COMP_DECL_0bc(b,c,l)   DIM1(p.m_a, l, nullptr, EXPR(b), EXPR(c))
#define ARRAY_COMP_DECL_abc(a,b,c,l) DIM1(p.m_a, l, EXPR(a), EXPR(b), EXPR(c))

#define ARRAY_COMP_DECL1k(id, a, l)   DIM1k(p.m_a, l, id, EXPR(INTEGER(1, l)), EXPR(a))

#define ARRAY_COMP_DECL1d(a, l)       DIM1d(p.m_a, l, EXPR(INTEGER(1, l)), EXPR(a))
#define ARRAY_COMP_DECL2d(a, b, l)    DIM1d(p.m_a, l, EXPR(a), EXPR(b))
#define ARRAY_COMP_DECL3d(a, l)       DIM1d(p.m_a, l, EXPR(a), nullptr)
#define ARRAY_COMP_DECL4d(b, l)       DIM1d(p.m_a, l, nullptr, EXPR(b))
#define ARRAY_COMP_DECL5d(l)          DIM1d(p.m_a, l, nullptr, nullptr)
#define ARRAY_COMP_DECL6d(l)          DIM1d_star(p.m_a, l, nullptr)
#define ARRAY_COMP_DECL7d(a, l)       DIM1d_star(p.m_a, l, EXPR(a))

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

#define FN_MOD1(a, l) make_FnMod_t(p.m_a, l, \
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
        const Vec<FnArg> &args,
        const Vec<struct_member_t> &member,
        Location &l) {
    Vec<fnarg_t> v;
    v.reserve(al, args.size());
    Vec<keyword_t> v2;
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
        member.p, member.size(),
        /*expr_t** a_args*/ v.p, /*size_t n_args*/ v.size(),
        /*keyword_t* a_keywords*/ v2.p, /*size_t n_keywords*/ v2.size());
}

#define FUNCCALLORARRAY(id, args, l) FUNCCALLORARRAY0(p.m_a, id, args, \
        empty5(), l)
#define FUNCCALLORARRAY2(members, id, args, l) FUNCCALLORARRAY0(p.m_a, id, \
        args, members, l)

#define SELECT(cond, body, def, l) make_Select_t(p.m_a, l, 0, nullptr, \
        EXPR(cond), \
        CASE_STMTS(body), body.size(), \
        STMTS(def), def.size())

#define CASE_STMT(cond, body, l) make_CaseStmt_t(p.m_a, l, \
        EXPRS(cond), cond.size(), STMTS(body), body.size())
#define CASE_STMT2(cond, body, l) make_CaseStmt_Range_t(p.m_a, l, \
        EXPR(cond), nullptr, STMTS(body), body.size())
#define CASE_STMT3(cond, body, l) make_CaseStmt_Range_t(p.m_a, l, \
        nullptr, EXPR(cond), STMTS(body), body.size())
#define CASE_STMT4(cond1, cond2, body, l) make_CaseStmt_Range_t(p.m_a, l, \
        EXPR(cond1), EXPR(cond2), STMTS(body), body.size())

#define SELECT_TYPE1(sel, body, l) make_SelectType_t(p.m_a, l, 0, nullptr, \
        nullptr, EXPR(sel), TYPE_STMTS(body), body.size())
#define SELECT_TYPE2(id, sel, body, l) make_SelectType_t(p.m_a, l, 0, nullptr, \
        name2char(id), EXPR(sel), \
        TYPE_STMTS(body), body.size())

#define TYPE_STMTNAME(x, body, l) make_TypeStmtName_t(p.m_a, l, \
        x.c_str(p.m_a), STMTS(body), body.size())
#define TYPE_STMTVAR(vartype, body, l) make_TypeStmtType_t(p.m_a, l, \
        down_cast<decl_attribute_t>(vartype), STMTS(body), body.size())
#define CLASS_STMT(id, body, l) make_ClassStmt_t(p.m_a, l, \
        name2char(id), STMTS(body), body.size())
#define CLASS_DEFAULT(body, l) make_ClassDefault_t(p.m_a, l, \
        STMTS(body), body.size())

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

#define MODULE(name, use, implicit, decl, contains, l) make_Module_t(p.m_a, l, \
        name2char(name), \
        /*unit_decl1_t** a_use*/ USES(use), /*size_t n_use*/ use.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*unit_decl2_t** a_decl*/ DECLS(decl), /*size_t n_decl*/ decl.size(), \
        /*program_unit_t** a_contains*/ CONTAINS(contains), /*size_t n_contains*/ contains.size())
#define SUBMODULE(id ,name, use, implicit, decl, contains, l) make_Submodule_t(p.m_a, l, \
        name2char(id), \
        name2char(name), \
        /*unit_decl1_t** a_use*/ USES(use), /*size_t n_use*/ use.size(), \
        /*m_implicit*/ VEC_CAST(implicit, implicit_statement), \
        /*n_implicit*/ implicit.size(), \
        /*unit_decl2_t** a_decl*/ DECLS(decl), /*size_t n_decl*/ decl.size(), \
        /*program_unit_t** a_contains*/ CONTAINS(contains), /*size_t n_contains*/ contains.size())
#define PRIVATE0(l) make_Private_t(p.m_a, l, \
        nullptr, 0)
#define PRIVATE(syms, l) make_Private_t(p.m_a, l, \
        nullptr, 0)
#define PUBLIC(syms, l) make_Public_t(p.m_a, l, \
        nullptr, 0)

#define INTERFACE_HEADER(l) make_InterfaceHeader_t(p.m_a, l)
#define INTERFACE_HEADER_NAME(id, l) make_InterfaceHeaderName_t(p.m_a, l, \
        name2char(id))
#define INTERFACE_HEADER_ASSIGNMENT(l) make_InterfaceHeaderAssignment_t(p.m_a, l)
#define INTERFACE_HEADER_OPERATOR(op, l) make_InterfaceHeaderOperator_t(p.m_a, l, op)
#define INTERFACE_HEADER_CUSTOMOP(op, l) make_InterfaceHeaderCustomOperator_t(p.m_a, \
        l, op.c_str(p.m_a))
#define ABSTRACT_INTERFACE_HEADER(l) make_AbstractInterfaceHeader_t(p.m_a, l)

#define OPERATOR(op, l) interfaceopType::op

#define INTERFACE(header, contains, l) make_Interface_t(p.m_a, l, \
        down_cast<interface_header_t>(header), INTERFACE_ITEMS(contains), contains.size())
#define INTERFACE_MODULE_PROC(names, l) \
        make_InterfaceModuleProcedure_t(p.m_a, l, \
        REDUCE_ARGS(p.m_a, names), names.size())
#define INTERFACE_PROC(proc, l) \
        make_InterfaceProc_t(p.m_a, l, \
        down_cast<program_unit_t>(proc))

// TODO: Add DerivedType AST node
#define DERIVED_TYPE(name, decl, l) make_DerivedType_t(p.m_a, l, \
        name2char(name), DECLS(decl), decl.size())

#endif
