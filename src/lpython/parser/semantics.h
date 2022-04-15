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

#define RESULT(x) p.result.push_back(p.m_a, STMT(x))
#define LIST_NEW(l) l.reserve(p.m_a, 4)
#define LIST_ADD(l, x) l.push_back(p.m_a, x)
#define PLIST_ADD(l, x) l.push_back(p.m_a, *x)

#define OPERATOR(op, l) operatorType::op
#define AUGASSIGN_01(x, op, y, l) make_AugAssign_t(p.m_a, l, EXPR(x), op, EXPR(y))

#define ANNASSIGN_01(x, y, l) make_AnnAssign_t(p.m_a, l, \
        EXPR(x), EXPR(y), nullptr, 1)
#define ANNASSIGN_02(x, y, val, l) make_AnnAssign_t(p.m_a, l, \
        EXPR(x), EXPR(y), EXPR(val), 1)

#define DELETE(e, l) make_Delete_t(p.m_a, l, EXPRS(e), e.size())
#define DEL_TARGET_ID(name, l) make_Name_t(p.m_a, l, \
        name2char(name), expr_contextType::Del)

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

#define ASSIGNMENT(targets, val, l) make_Assign_t(p.m_a, l, \
        EXPRS(targets), targets.size(), EXPR(val), nullptr)
#define TARGET_ID(name, l) make_Name_t(p.m_a, l, \
        name2char(name), expr_contextType::Store)
/*
#define TARGET_EXPR_ID(e, id, l) make_Attribute_t(p.m_a, l, \
        EXPR(e), name2char(id), expr_contextType::Store)
*/
#define TUPLE_01(elts, l) make_Tuple_t(p.m_a, l, \
        EXPRS(elts), elts.size(), expr_contextType::Store)

static inline alias_t *IMPORT_ALIAS_01(Allocator &al, Location &l,
        char *name, char *asname){
    alias_t *r = al.allocate<alias_t>();
    r->loc = l;
    r->m_name = name;
    r->m_asname = asname;
    return r;
}

static inline char *mod2char(Allocator &al, Vec<ast_t*> module) {
    std::string s = "";
    for (size_t i=0; i<module.size(); i++) {
        s.append(name2char(module[i]));
        if (i < module.size()-1)s.append(".");
    }
    LFortran::Str str;
    str.from_str_view(s);
    return str.c_str(al);
}

#define MOD_ID_01(module, l) IMPORT_ALIAS_01(p.m_a, l, \
        mod2char(p.m_a, module), nullptr)
#define MOD_ID_02(module, as_id, l) IMPORT_ALIAS_01(p.m_a, l, \
        mod2char(p.m_a, module), name2char(as_id))
#define MOD_ID_03(star, l) IMPORT_ALIAS_01(p.m_a, l, \
        (char *)"*", nullptr)

int dot_count = 0;
#define DOT_COUNT_01() dot_count++;
#define DOT_COUNT_02() dot_count = dot_count + 3;

#define IMPORT_01(names, l) make_Import_t(p.m_a, l, names.p, names.size())
#define IMPORT_02(module, names, l) make_ImportFrom_t(p.m_a, l, \
        mod2char(p.m_a, module), names.p, names.size(), 0)
#define IMPORT_03(names, l) make_ImportFrom_t(p.m_a, l, \
        nullptr, names.p, names.size(), dot_count); dot_count = 0
#define IMPORT_04(module, names, l) make_ImportFrom_t(p.m_a, l, \
        mod2char(p.m_a, module), names.p, names.size(), dot_count); \
        dot_count = 0

#define BINOP(x, op, y, l) make_BinOp_t(p.m_a, l, \
        EXPR(x), operatorType::op, EXPR(y))
#define UNARY(x, op, l) make_UnaryOp_t(p.m_a, l, unaryopType::op, EXPR(x))
#define COMPARE(x, op, y, l) make_Compare_t(p.m_a, l, \
        EXPR(x), cmpopType::op, EXPRS(A2LIST(p.m_a, y)), 1)

#define SYMBOL(x, l) make_Name_t(p.m_a, l, \
        x.c_str(p.m_a), expr_contextType::Load)
// `x.int_n` is of type BigInt but we store the int64_t directly in AST
#define INTEGER(x, l) make_ConstantInt_t(p.m_a, l, \
        std::stoi(x.int_n.str()), nullptr)
#define STRING(x, l) make_ConstantStr_t(p.m_a, l, x.c_str(p.m_a), nullptr)
#define FLOAT(x, l) make_ConstantFloat_t(p.m_a, l, \
        std::stof(x.c_str(p.m_a)), nullptr)
#define COMPLEX(x, l) make_ConstantComplex_t(p.m_a, l, \
        0, std::stof(x.int_n.str()), nullptr)
#define BOOL(x, l) make_ConstantBool_t(p.m_a, l, x, nullptr)
#define CALL_01(func, l) make_Call_t(p.m_a, l, \
        EXPR(func), nullptr, 0, nullptr, 0)
#define CALL_02(func, args, l) make_Call_t(p.m_a, l, \
        EXPR(func), EXPRS(args), args.size(), nullptr, 0)


#endif
