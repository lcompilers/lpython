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
using LFortran::Key_Val;

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

#define EXPR_01(e, l) make_Expr_t(p.m_a, l, EXPR(e))

#define RETURN_01(l) make_Return_t(p.m_a, l, nullptr)
#define RETURN_02(e, l) make_Return_t(p.m_a, l, EXPR(e))

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
#define TARGET_ATTR(val, attr, l) make_Attribute_t(p.m_a, l, \
        EXPR(val), name2char(attr), expr_contextType::Store)
#define TARGET_SUBSCRIPT(value, slice, l) make_Subscript_t(p.m_a, l, \
        EXPR(value), CHECK_TUPLE(EXPR(slice)), expr_contextType::Store)
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

#define IF_01(test, body, l) make_If_t(p.m_a, l, \
        /*test*/ EXPR(test), \
        /*body*/ STMTS(A2LIST(p.m_a, body)), \
        /*n_body*/ 1, \
        /*a_orelse*/ nullptr, \
        /*n_orelse*/ 0)

#define IF_STMT_01(e, stmt, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), nullptr, 0)
#define IF_STMT_02(e, stmt, orelse, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), STMTS(orelse), orelse.size())
#define IF_STMT_03(e, stmt, orelse, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), STMTS(A2LIST(p.m_a, orelse)), 1)

#define FOR_01(target, iter, stmts, l) make_For_t(p.m_a, l, \
        EXPR(target), EXPR(iter), STMTS(stmts), stmts.size(), \
        nullptr, 0, nullptr)
#define FOR_02(target, iter, stmts, orelse, l) make_For_t(p.m_a, l, \
        EXPR(target), EXPR(iter), STMTS(stmts), stmts.size(), \
        STMTS(orelse), orelse.size(), nullptr)

#define TRY_01(stmts, except, l) make_Try_t(p.m_a, l, \
        STMTS(stmts), stmts.size(), \
        VEC_CAST(except, excepthandler), except.size(), nullptr, 0, nullptr, 0)
#define TRY_02(stmts, except, orelse, l) make_Try_t(p.m_a, l, \
        STMTS(stmts), stmts.size(), \
        VEC_CAST(except, excepthandler), except.size(), \
        STMTS(orelse), orelse.size(), nullptr, 0)
#define TRY_03(stmts, except, final, l) make_Try_t(p.m_a, l, \
        STMTS(stmts), stmts.size(), \
        VEC_CAST(except, excepthandler), except.size(), nullptr, 0, \
        STMTS(final), final.size())
#define TRY_04(stmts, except, orelse, final, l) make_Try_t(p.m_a, l, \
        STMTS(stmts), stmts.size(), \
        VEC_CAST(except, excepthandler), except.size(), \
        STMTS(orelse), orelse.size(), STMTS(final), final.size())
#define TRY_05(stmts, final, l) make_Try_t(p.m_a, l, \
        STMTS(stmts), stmts.size(), \
        nullptr, 0, nullptr, 0, STMTS(final), final.size())
#define EXCEPT_01(stmts, l) make_ExceptHandler_t(p.m_a, l, \
        nullptr, nullptr, STMTS(stmts), stmts.size())
#define EXCEPT_02(e, stmts, l) make_ExceptHandler_t(p.m_a, l, \
        EXPR(e), nullptr, STMTS(stmts), stmts.size())
#define EXCEPT_03(e, id, stmts, l) make_ExceptHandler_t(p.m_a, l, \
        EXPR(e), name2char(id), STMTS(stmts), stmts.size())

static inline withitem_t *WITH_ITEM(Allocator &al, Location &l,
        expr_t* context_expr, expr_t* optional_vars) {
    withitem_t *r = al.allocate<withitem_t>();
    r->loc = l;
    r->m_context_expr = context_expr;
    r->m_optional_vars = optional_vars;
    return r;
}

#define WITH_ITEM_01(expr, l) WITH_ITEM(p.m_a, l, EXPR(expr), nullptr)
#define WITH_ITEM_02(expr, vars, l) WITH_ITEM(p.m_a, l, EXPR(expr), EXPR(vars))
#define WITH(items, body, l) make_With_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), nullptr)

static inline arg_t *FUNC_ARG(Allocator &al, Location &l, char *arg, expr_t* e) {
    arg_t *r = al.allocate<arg_t>();
    r->loc = l;
    r->m_arg = arg;
    r->m_annotation = e;
    r->m_type_comment = nullptr;
    return r;
}

static inline arguments_t FUNC_ARGS(Location &l,
    arg_t* m_posonlyargs, size_t n_posonlyargs, arg_t* m_args, size_t n_args,
    arg_t* m_vararg, size_t n_vararg, arg_t* m_kwonlyargs, size_t n_kwonlyargs,
    expr_t** m_kw_defaults, size_t n_kw_defaults, arg_t* m_kwarg, size_t n_kwarg,
    expr_t** m_defaults, size_t n_defaults) {
    arguments_t r;
    r.loc = l;
    r.m_posonlyargs = m_posonlyargs;
    r.n_posonlyargs = n_posonlyargs;
    r.m_args = m_args;
    r.n_args = n_args;
    r.m_vararg = m_vararg;
    r.n_vararg = n_vararg;
    r.m_kwonlyargs = m_kwonlyargs;
    r.n_kwonlyargs = n_kwonlyargs;
    r.m_kw_defaults = m_kw_defaults;
    r.n_kw_defaults = n_kw_defaults;
    r.m_kwarg = m_kwarg;
    r.n_kwarg = n_kwarg;
    r.m_defaults = m_defaults;
    r.n_defaults = n_defaults;
    return r;
}

#define ARGS_01(arg, l) FUNC_ARG(p.m_a, l, name2char((ast_t *)arg), nullptr)
#define ARGS_02(arg, annotation, l) FUNC_ARG(p.m_a, l, \
        name2char((ast_t *)arg), EXPR(annotation))
#define FUNCTION_01(decorator, id, args, stmts, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size(), \
        nullptr, nullptr)
#define FUNCTION_02(decorator, id, args, return, stmts, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size(), \
        EXPR(return), nullptr)

#define CLASS_01(decorator, id, stmts, l) make_ClassDef_t(p.m_a, l, \
        name2char(id), nullptr, 0, nullptr, 0, \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size())
#define CLASS_02(decorator, id, args, stmts, l) make_ClassDef_t(p.m_a, l, \
        name2char(id), EXPRS(args), args.size(), nullptr, 0, \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size())

#define ASYNC_FUNCTION_01(decorator, id, args, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size(), \
        nullptr, nullptr)
#define ASYNC_FUNCTION_02(decorator, id, args, return, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size(), \
        EXPR(return), nullptr)
#define ASYNC_FUNCTION_03(id, args, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        nullptr, 0, nullptr, nullptr)
#define ASYNC_FUNCTION_04(id, args, return, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), \
        FUNC_ARGS(l, nullptr, 0, args.p, args.n, nullptr, 0, \
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0), \
        STMTS(stmts), stmts.size(), \
        nullptr, 0, EXPR(return), nullptr)

#define ASYNC_FOR_01(target, iter, stmts, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(target), EXPR(iter), STMTS(stmts), stmts.size(), \
        nullptr, 0, nullptr)
#define ASYNC_FOR_02(target, iter, stmts, orelse, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(target), EXPR(iter), STMTS(stmts), stmts.size(), \
        STMTS(orelse), orelse.size(), nullptr)

#define ASYNC_WITH(items, body, l) make_AsyncWith_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), nullptr)

Vec<ast_t*> MERGE_EXPR(Allocator &al, ast_t *x, ast_t *y) {
    Vec<ast_t*> v;
    v.reserve(al, 2);
    v.push_back(al, x);
    v.push_back(al, y);
    return v;
}

#define BOOLOP(x, op, y, l) make_BoolOp_t(p.m_a, l, \
        boolopType::op, EXPRS(MERGE_EXPR(p.m_a, x, y)), 2)
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
#define CALL_01(func, args, l) make_Call_t(p.m_a, l, \
        EXPR(func), EXPRS(args), args.size(), nullptr, 0)
#define LIST(e, l) make_List_t(p.m_a, l, \
        EXPRS(e), e.size(), expr_contextType::Load)
#define ATTRIBUTE_REF(val, attr, l) make_Attribute_t(p.m_a, l, \
        EXPR(val), name2char(attr), expr_contextType::Load)

expr_t* CHECK_TUPLE(expr_t *x) {
    if(is_a<Tuple_t>(*x) && down_cast<Tuple_t>(x)->n_elts == 1) {
        return down_cast<Tuple_t>(x)->m_elts[0];
    } else {
        return x;
    }
}

#define TUPLE(elts, l) make_Tuple_t(p.m_a, l, \
        EXPRS(elts), elts.size(), expr_contextType::Load)
#define SUBSCRIPT(value, slice, l) make_Subscript_t(p.m_a, l, \
        EXPR(value), CHECK_TUPLE(EXPR(slice)), expr_contextType::Load)

Key_Val *DICT(Allocator &al, expr_t *key, expr_t *val) {
    Key_Val *kv = al.allocate<Key_Val>();
    kv->key = key;
    kv->value = val;
    return kv;
}

ast_t *DICT1(Allocator &al, Location &l, Vec<Key_Val*> dict_list) {
    Vec<expr_t*> key;
    key.reserve(al, dict_list.size());
    Vec<expr_t*> val;
    val.reserve(al, dict_list.size());
    for (auto &item : dict_list) {
        key.push_back(al, item->key);
        val.push_back(al, item->value);
    }
    return make_Dict_t(al, l, key.p, key.n, val.p, val.n);
}

#define DICT_EXPR(key, value, l) DICT(p.m_a, EXPR(key), EXPR(value))
#define DICT_01(l) make_Dict_t(p.m_a, l, nullptr, 0, nullptr, 0)
#define DICT_02(dict_list, l) DICT1(p.m_a, l, dict_list)

#endif
