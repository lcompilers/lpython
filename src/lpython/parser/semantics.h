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
#include <lpython/parser/parser_exception.h>

// This is only used in parser.tab.cc, nowhere else, so we simply include
// everything from LCompilers::LPython::AST to save typing:
using namespace LCompilers::LPython::AST;
using LCompilers::Location;
using LCompilers::Vec;
using LCompilers::LPython::Key_Val;
using LCompilers::LPython::Args;
using LCompilers::LPython::Arg;
using LCompilers::LPython::Fn_Arg;
using LCompilers::LPython::Args_;
using LCompilers::LPython::Var_Kw;
using LCompilers::LPython::Kw_or_Star_Arg;
using LCompilers::LPython::Call_Arg;
using LCompilers::LPython::Key_Val_Pattern;

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
        LCOMPILERS_ASSERT((s[i]->base.type == type))
    }
    return s;
}

// Assign first/last location to `a` from `b`
#define FLOC(a, b) a.first = b.first;
#define LLOC(a, b) a.last = b.last;


#define VEC_CAST(x, type) vec_cast<type##_t, astType::type>(x)
#define STMTS(x) VEC_CAST(x, stmt)
#define EXPRS(x) VEC_CAST(x, expr)

#define EXPR(x) (down_cast<expr_t>(x))
#define STMT(x) (down_cast<stmt_t>(x))
#define EXPR2STMT(x) ((stmt_t*)make_Expr_t(p.m_a, x->base.loc, x))

static inline expr_t* EXPR_OPT(const ast_t *f)
{
    if (f) {
        return EXPR(f);
    } else {
        return nullptr;
    }
}

#define SET_EXPR_CTX_(y, ctx) \
        case exprType::y: { \
            if(is_a<y##_t>(*EXPR(x))) \
                ((y##_t*)x)->m_ctx = ctx; \
            break; \
        }

static inline ast_t* SET_EXPR_CTX_01(ast_t* x, expr_contextType ctx) {
    LCOMPILERS_ASSERT(is_a<expr_t>(*x))
    switch(EXPR(x)->type) {
        SET_EXPR_CTX_(Attribute, ctx)
        SET_EXPR_CTX_(Subscript, ctx)
        SET_EXPR_CTX_(Starred, ctx)
        SET_EXPR_CTX_(Name, ctx)
        SET_EXPR_CTX_(List, ctx)
        SET_EXPR_CTX_(Tuple, ctx)
        default : { break; }
    }
    return x;
}
static inline Vec<ast_t*> SET_EXPR_CTX_02(Vec<ast_t*> x, expr_contextType ctx) {
    for (size_t i=0; i < x.size(); i++) {
        SET_EXPR_CTX_01(x[i], ctx);
    }
    return x;
}

#define RESULT(x) p.result.push_back(p.m_a, STMT(x))

#define LIST_NEW(l) l.reserve(p.m_a, 4)
#define LIST_ADD(l, x) l.push_back(p.m_a, x)
#define PLIST_ADD(l, x) l.push_back(p.m_a, *x)

#define OPERATOR(op, l) operatorType::op
#define AUGASSIGN_01(x, op, y, l) make_AugAssign_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(x, Store)), op, EXPR(y))

#define ANNASSIGN_01(x, y, l) make_AnnAssign_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(x, Store)), EXPR(y), nullptr, 1)
#define ANNASSIGN_02(x, y, val, l) make_AnnAssign_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(x, Store)), EXPR(y), EXPR(val), 1)

#define DELETE_01(e, l) make_Delete_t(p.m_a, l, \
        EXPRS(SET_EXPR_CTX_02(SET_CTX_02(e, Del), Del)), e.size())

#define EXPR_01(e, l) make_Expr_t(p.m_a, l, EXPR(e))

#define RETURN_01(l) make_Return_t(p.m_a, l, nullptr)
#define RETURN_02(e, l) make_Return_t(p.m_a, l, EXPR(e))

#define YIELD_01(l) make_Yield_t(p.m_a, l, nullptr)
#define YIELD_02(exec, l) make_Yield_t(p.m_a, l, EXPR(exec))
#define YIELD_03(value, l) make_YieldFrom_t(p.m_a, l, EXPR(value))

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

static inline ast_t *SET_CTX_01(ast_t *x, expr_contextType ctx) {
    if(is_a<Tuple_t>(*EXPR(x))) {
        size_t n_elts = down_cast2<Tuple_t>(x)->n_elts;
        for(size_t i=0; i < n_elts; i++) {
            SET_EXPR_CTX_01((ast_t *) down_cast2<Tuple_t>(x)->m_elts[i], ctx);
        }
    }
    return x;
}
static inline Vec<ast_t*> SET_CTX_02(Vec<ast_t*> x, expr_contextType ctx) {
    for (size_t i=0; i < x.size(); i++) {
        SET_CTX_01(x[i], ctx);
    }
    return x;
}
#define ASSIGNMENT(targets, val, l) make_Assign_t(p.m_a, l, \
        EXPRS(SET_EXPR_CTX_02(SET_CTX_02(targets, Store), Store)), targets.size(), \
        EXPR(val), nullptr)
#define ASSIGNMENT2(targets, val, type_comment, l) make_Assign_t(p.m_a, l, \
        EXPRS(SET_EXPR_CTX_02(SET_CTX_02(targets, Store), Store)), targets.size(), \
        EXPR(val), extract_type_comment(p, l, type_comment))

static inline ast_t* TUPLE_02(Allocator &al, Location &l, Vec<ast_t*> elts) {
    if(is_a<expr_t>(*elts[0]) && elts.size() == 1) {
        return (ast_t*) elts[0];
    }
    return make_Tuple_t(al, l, EXPRS(elts), elts.size(), expr_contextType::Load);
}
#define TUPLE_01(elts, l) TUPLE_02(p.m_a, l, elts)
#define TUPLE_03(elts, l) make_Tuple_t(p.m_a, l, \
        EXPRS(elts), elts.size(), expr_contextType::Load)
#define TUPLE_EMPTY(l) make_Tuple_t(p.m_a, l, \
        nullptr, 0, expr_contextType::Load)

Vec<ast_t*> TUPLE_APPEND(Allocator &al, Vec<ast_t*> x, ast_t *y) {
    Vec<ast_t*> v;
    v.reserve(al, x.size()+1);
    for (size_t i=0; i<x.size(); i++) {
        v.push_back(al, x[i]);
    }
    v.push_back(al, y);
    return v;
}
#define TUPLE_(x, y) TUPLE_APPEND(p.m_a, x, y)

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
    LCompilers::Str str;
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

#define IF_STMT_01(e, stmt, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), nullptr, 0)
#define IF_STMT_02(e, stmt, orelse, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), STMTS(orelse), orelse.size())
#define IF_STMT_03(e, stmt, orelse, l) make_If_t(p.m_a, l, \
        EXPR(e), STMTS(stmt), stmt.size(), STMTS(A2LIST(p.m_a, orelse)), 1)
#define TERNARY(test, body, orelse, l) make_IfExp_t(p.m_a, l, \
        EXPR(test), EXPR(body), EXPR(orelse))

static inline char *extract_type_comment(LCompilers::LPython::Parser &p,
        Location &loc, LCompilers::Str &s) {
    std::string str = s.str();

    str.erase(str.begin()); // removes "#" at the beginning
    str.erase(0, str.find_first_not_of(' ')); // trim left spaces

    std::string::size_type pos = 5;
    str = str.substr(pos, str.size());
    str.erase(str.find_last_not_of(' ') + 1); // trim right spaces
    str.erase(0, str.find_first_not_of(' ')); // trim left spaces

    size_t i = 0;
    bool flag = true;
    std::string kw{"ignore"};
    while (i < 6) {
        if(str[i] == kw[i]) {
            i++;
        } else {
            flag = false;
            break;
        }
    }

    if (flag) {
        pos = 6;
        str = str.substr(pos, str.size());
        s.from_str_view(str);
        p.type_ignore.push_back(p.m_a, down_cast<type_ignore_t>(
            make_TypeIgnore_t(p.m_a, loc, 0, s.c_str(p.m_a))));
        return nullptr;
    } else {
        s.from_str_view(str);
        return s.c_str(p.m_a);
    }
}

#define FOR_01(target, iter, stmts, l) make_For_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), nullptr, 0, nullptr)
#define FOR_02(target, iter, stmts, orelse, l) make_For_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), STMTS(orelse), orelse.size(), nullptr)
#define FOR_03(target, iter, type_comment, stmts, l) make_For_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), nullptr, 0, \
        extract_type_comment(p, l, type_comment))
#define FOR_04(target, iter, stmts, orelse, type_comment, l) make_For_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), STMTS(orelse), orelse.size(), \
        extract_type_comment(p, l, type_comment))

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

#define MATCH_VALUE(val, l) down_cast<pattern_t>( \
        make_MatchValue_t(p.m_a, l, EXPR(val)))

#define MATCH_SINGLETON(val, l) down_cast<pattern_t>( \
        make_MatchSingleton_t(p.m_a, l, EXPR(val)))

#define MATCH_AS_01(pattern, id, l) down_cast<pattern_t>( \
        make_MatchAs_t(p.m_a, l, pattern, name2char(id)))
#define MATCH_AS_02(id, l) down_cast<pattern_t>( \
        make_MatchAs_t(p.m_a, l, nullptr, name2char(id)))

#define MATCH_OR(v, l) down_cast<pattern_t>(make_MatchOr_t(p.m_a, l, v.p, v.n))

#define MATCH_STAR(id, l) down_cast<pattern_t>( \
        make_MatchStar_t(p.m_a, l, name2char(id)))
#define MATCH_SEQUENCE_01(l) down_cast<pattern_t>( \
        make_MatchSequence_t(p.m_a, l, nullptr, 0))
#define MATCH_SEQUENCE_02(patterns, l) down_cast<pattern_t>( \
        make_MatchSequence_t(p.m_a, l, patterns.p, patterns.n))

static inline Key_Val_Pattern* KEY_VAL_PATTERN(Allocator &al,
        expr_t* key,  pattern_t* value) {
    Key_Val_Pattern* r = al.allocate<Key_Val_Pattern>();
    r->key = key;
    r->val = value;
    return r;
}
pattern_t *match_mapping(Allocator &al, Location &l,
        Vec<Key_Val_Pattern> items, char *rest) {
    Vec<expr_t*> key;
    key.reserve(al, items.size());
    Vec<pattern_t*> val;
    val.reserve(al, items.size());
    for (auto &item : items) {
        key.push_back(al, item.key);
        val.push_back(al, item.val);
    }
    return down_cast<pattern_t>(
            make_MatchMapping_t(al, l, key.p, key.n, val.p, val.n, rest));
}
#define MATCH_MAPPING_01(l) down_cast<pattern_t>( \
        make_MatchMapping_t(p.m_a, l, nullptr, 0, nullptr, 0, nullptr))
#define MATCH_MAPPING_02(items, l) match_mapping(p.m_a, l, items, nullptr)
#define MATCH_MAPPING_03(items, rest, l) match_mapping(p.m_a, l, \
        items, name2char(rest))
#define MATCH_MAPPING_04(rest, l) down_cast<pattern_t>( \
        make_MatchMapping_t(p.m_a, l, nullptr, 0, nullptr, 0, name2char(rest)))

pattern_t *match_class(Allocator &al, Location &l, expr_t *cls,
        pattern_t** m_patterns, size_t n_patterns,
        Key_Val_Pattern* m_params, size_t n_params) {
    Vec<char*> kwd_attrs;
    kwd_attrs.reserve(al, n_params);
    Vec<pattern_t*> kwd_patterns;
    kwd_patterns.reserve(al, n_params);
    for (size_t i = 0; i < n_params; i++) {
        kwd_attrs.push_back(al, name2char((ast_t*)m_params[i].key));
        kwd_patterns.push_back(al, m_params[i].val);
    }
    return down_cast<pattern_t>(make_MatchClass_t(al, l, cls,
        m_patterns, n_patterns, kwd_attrs.p, kwd_attrs.n,
        kwd_patterns.p, kwd_patterns.n));
}
#define MATCH_CLASS_01(cls, l) down_cast<pattern_t>( \
        make_MatchClass_t(p.m_a, l, EXPR(cls), nullptr,0, \
        nullptr, 0, nullptr, 0))
#define MATCH_CLASS_02(cls, args, l) match_class(p.m_a, l, \
        EXPR(cls), args.p, args.n, nullptr, 0)
#define MATCH_CLASS_03(cls, args, kws, l) match_class(p.m_a, l, \
        EXPR(cls), args.p, args.n, kws.p, kws.n)
#define MATCH_CLASS_04(cls, kws, l) match_class(p.m_a, l, \
        EXPR(cls), nullptr, 0, kws.p, kws.n)

static inline match_case_t* match_case(Allocator &al, Location &l,
        pattern_t* pattern, expr_t* guard, Vec<ast_t*> body) {
    match_case_t* r = al.allocate<match_case_t>();
    r->loc = l;
    r->m_pattern = pattern;
    r->m_guard = guard;
    r->m_body = STMTS(body);
    r->n_body = body.size();
    return r;
}
#define MATCH_CASE_01(pattern, body, l) match_case(p.m_a, l, \
        pattern, nullptr, body)
#define MATCH_CASE_02(pattern, guard, body, l) match_case(p.m_a, l, \
        pattern, EXPR(guard), body)
#define MATCH_01(subject, cases, l) make_Match_t(p.m_a, l, \
        EXPR(subject), cases.p, cases.n)

static inline withitem_t* WITH_ITEM(Allocator &al, Location &l,
        expr_t* context_expr, expr_t* optional_vars) {
    withitem_t* r = al.allocate<withitem_t>();
    r->loc = l;
    r->m_context_expr = context_expr;
    r->m_optional_vars = optional_vars;
    return r;
}

#define WITH_ITEM_01(expr, vars, l) WITH_ITEM(p.m_a, l, \
        EXPR(expr), EXPR(SET_EXPR_CTX_01(vars, Store)))
#define WITH_ITEM_02(expr, l) WITH_ITEM(p.m_a, l, EXPR(expr), nullptr)
#define WITH_01(items, body, l) make_With_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), nullptr)
#define WITH_02(items, body, type_comment, l) make_With_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), \
        extract_type_comment(p, l, type_comment))

static inline Arg *FUNC_ARG(Allocator &al, Location &l, char *arg,
        expr_t* e, expr_t* defaults) {
    Arg *r = al.allocate<Arg>();
    r->_arg.loc = l;
    r->_arg.m_arg = arg;
    r->_arg.m_annotation = e;
    r->_arg.m_type_comment = nullptr;
    if(defaults){
        r->default_value = true;
    } else {
        r->default_value = false;
    }
    r->defaults = defaults;
    return r;
}

Arg** ARG2LIST(Allocator &al, Arg *x) {
    Arg** v = al.allocate<Arg*>(1);
    v[0] = x;
    return v;
}

#define FUNC_ARGS1_(x) \
        Vec<arg_t> _m_##x; \
        _m_##x.reserve(al, 4); \
        r->arguments.m_##x = _m_##x.p; \
        r->arguments.n_##x = _m_##x.n;

#define FUNC_ARGS_(x, kw) \
        if(n_##x > 0) { \
            for(size_t i = 0; i < n_##x; i++) { \
                _m_##x.push_back(al, m_##x[i]->_arg); \
                if(m_##x[i]->default_value) { \
                    if (kw == 0) { \
                        defaults.push_back(al, m_##x[i]->defaults); \
                    } else { \
                        kw_defaults.push_back(al, m_##x[i]->defaults); \
                    } \
                } else { \
                    if (kw == 1) { \
                        kw_defaults.push_back(al, \
                            (expr_t*)make_ConstantNone_t(al, l, nullptr)); \
                    } \
                } \
            } \
            r->arguments.m_##x = _m_##x.p; \
            r->arguments.n_##x = _m_##x.n; \
        }

static inline Args *FUNC_ARGS_01(Allocator &al, Location &l, Fn_Arg *parameters) {
    Args *r = al.allocate<Args>();
    Vec<expr_t*> defaults;
    defaults.reserve(al, 4);
    Vec<expr_t*> kw_defaults;
    kw_defaults.reserve(al, 4);

    FUNC_ARGS1_(posonlyargs);
    FUNC_ARGS1_(args);
    FUNC_ARGS1_(vararg);
    FUNC_ARGS1_(kwonlyargs);
    FUNC_ARGS1_(kwarg);

    r->arguments.loc = l;
    if(parameters != nullptr) {
        Arg** m_posonlyargs = parameters->posonlyargs.p;
        size_t n_posonlyargs = parameters->posonlyargs.n;
        FUNC_ARGS_(posonlyargs, 0);
    }
    if(parameters != nullptr && parameters->args_val) {
        Arg** m_args = parameters->args->args.p;
        size_t n_args = parameters->args->args.n;
        FUNC_ARGS_(args, 0);

        if(parameters->args->var_kw_val) {
            Arg** m_vararg = parameters->args->var_kw->vararg.p;
            size_t n_vararg = parameters->args->var_kw->vararg.n;
            FUNC_ARGS_(vararg, 0);

            Arg** m_kwonlyargs = parameters->args->var_kw->kwonlyargs.p;
            size_t n_kwonlyargs = parameters->args->var_kw->kwonlyargs.n;
            FUNC_ARGS_(kwonlyargs, 1);

            Arg** m_kwarg = parameters->args->var_kw->kwarg.p;
            size_t n_kwarg = parameters->args->var_kw->kwarg.n;
            FUNC_ARGS_(kwarg, 2);
        }
    }
    r->arguments.m_kw_defaults = kw_defaults.p;
    r->arguments.n_kw_defaults = kw_defaults.n;
    r->arguments.m_defaults = defaults.p;
    r->arguments.n_defaults = defaults.n;
    return r;
}

#define ARGS_01(arg, l) FUNC_ARG(p.m_a, l, \
        name2char((ast_t *)arg), nullptr, nullptr)
#define ARGS_02(arg, annotation, l) FUNC_ARG(p.m_a, l, \
        name2char((ast_t *)arg), EXPR(annotation), nullptr)
#define ARGS_03(arg, defaults, l) FUNC_ARG(p.m_a, l, \
        name2char((ast_t *)arg), nullptr, EXPR(defaults))
#define ARGS_04(arg, ann, defaults, l) FUNC_ARG(p.m_a, l, \
        name2char((ast_t *)arg), EXPR(ann), EXPR(defaults))

static inline void ADD_TYPE_COMMENT_(LCompilers::LPython::Parser &p, Location l,
        Vec<Arg*> &x, LCompilers::Str &type_comment) {
    x[x.size() - 1]->_arg.m_type_comment = extract_type_comment(p, l, type_comment);
}

#define ADD_TYPE_COMMENT(x, type_comment, l) \
        ADD_TYPE_COMMENT_(p, l, x, type_comment)

static inline Fn_Arg *FN_ARG(Allocator &al, Arg** m_posonlyargs,
        size_t n_posonlyargs, Args_ *args) {
    Fn_Arg *r = al.allocate<Fn_Arg>();
    r->posonlyargs.p = m_posonlyargs;
    r->posonlyargs.n = n_posonlyargs;
    if(args) {
        r->args_val = true;
    } else {
        r->args_val = false;
    }
    r->args = args;
    return r;
}

static inline Args_ *ARGS(Allocator &al, Arg** m_args,
        size_t n_args, Var_Kw *var_kw) {
    Args_ *r = al.allocate<Args_>();
    r->args.p = m_args;
    r->args.n = n_args;
    if(var_kw) {
        r->var_kw_val = true;
    } else {
        r->var_kw_val = false;
    }
    r->var_kw = var_kw;
    return r;
}

static inline Var_Kw *VAR_KW(Allocator &al,
        Arg** m_kwarg, size_t n_kwarg,
        Arg** m_kwonlyargs, size_t n_kwonlyargs,
        Arg** m_vararg, size_t n_vararg) {
    Var_Kw *r = al.allocate<Var_Kw>();
    r->kwarg.p = m_kwarg;
    r->kwarg.n = n_kwarg;
    r->kwonlyargs.p = m_kwonlyargs;
    r->kwonlyargs.n = n_kwonlyargs;
    r->vararg.p = m_vararg;
    r->vararg.n = n_vararg;
    return r;
}

#define PARAMETER_LIST_01(posonlyargs, args) \
        FN_ARG(p.m_a, posonlyargs.p, posonlyargs.n, args)
#define PARAMETER_LIST_02(args) FN_ARG(p.m_a, nullptr, 0, args)
#define PARAMETER_LIST_03(args, var_kw) ARGS(p.m_a, args.p, args.n, var_kw)
#define PARAMETER_LIST_04(var_kw) ARGS(p.m_a, nullptr, 0, var_kw)
#define PARAMETER_LIST_05(kwonlyargs) VAR_KW(p.m_a, nullptr, 0, \
        kwonlyargs.p, kwonlyargs.n, nullptr, 0)
#define PARAMETER_LIST_06(kwarg) VAR_KW(p.m_a, ARG2LIST(p.m_a, kwarg), 1, \
        nullptr, 0, nullptr, 0)
#define PARAMETER_LIST_07(kwonlyargs, kwarg) VAR_KW(p.m_a, \
        ARG2LIST(p.m_a, kwarg), 1, kwonlyargs.p, kwonlyargs.n, nullptr, 0)
#define PARAMETER_LIST_08(vararg) VAR_KW(p.m_a, nullptr, 0, \
        nullptr, 0, ARG2LIST(p.m_a, vararg), 1)
#define PARAMETER_LIST_09(vararg, kwonlyargs) VAR_KW(p.m_a, nullptr, 0, \
        kwonlyargs.p, kwonlyargs.n, ARG2LIST(p.m_a, vararg), 1)
#define PARAMETER_LIST_10(vararg, kwarg) VAR_KW(p.m_a, \
        ARG2LIST(p.m_a, kwarg), 1, nullptr, 0, ARG2LIST(p.m_a, vararg), 1)
#define PARAMETER_LIST_11(vararg, kwonlyargs, kwarg) VAR_KW(p.m_a, \
        ARG2LIST(p.m_a, kwarg), 1, kwonlyargs.p, kwonlyargs.n, \
        ARG2LIST(p.m_a, vararg), 1)
#define PARAMETER_LIST_12(l) FUNC_ARGS_01(p.m_a, l, nullptr)

#define FUNCTION_01(decorator, id, args, stmts, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        nullptr, nullptr)
#define FUNCTION_02(decorator, id, args, return, stmts, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        EXPR(return), nullptr)
#define FUNCTION_03(decorator, id, args, stmts, type_comment, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        nullptr, extract_type_comment(p, l, type_comment))
#define FUNCTION_04(decorator, id, args, return, stmts, type_comment, l) \
        make_FunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        EXPR(return), extract_type_comment(p, l, type_comment))

#define CLASS_01(decorator, id, stmts, l) make_ClassDef_t(p.m_a, l, \
        name2char(id), nullptr, 0, nullptr, 0, \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size())
#define CLASS_02(decorator, id, args, stmts, l) make_ClassDef_t(p.m_a, l, \
        name2char(id), args->expr.p, args->expr.n, args->kw.p, args->kw.n, \
        STMTS(stmts), stmts.size(), \
        EXPRS(decorator), decorator.size())

#define ASYNC_FUNCTION_01(decorator, id, args, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        nullptr, nullptr)
#define ASYNC_FUNCTION_02(decorator, id, args, return, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        EXPR(return), nullptr)
#define ASYNC_FUNCTION_03(id, args, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), nullptr, 0, nullptr, nullptr)
#define ASYNC_FUNCTION_04(id, args, return, stmts, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), nullptr, 0, EXPR(return), nullptr)
#define ASYNC_FUNCTION_05(decorator, id, args, stmts, type_comment, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        nullptr, extract_type_comment(p, l, type_comment))
#define ASYNC_FUNCTION_06(decorator, id, args, return, stmts, type_comment, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), EXPRS(decorator), decorator.size(), \
        EXPR(return), extract_type_comment(p, l, type_comment))
#define ASYNC_FUNCTION_07(id, args, stmts, type_comment, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), nullptr, 0, nullptr,\
        extract_type_comment(p, l, type_comment))
#define ASYNC_FUNCTION_08(id, args, return, stmts, type_comment, l) \
        make_AsyncFunctionDef_t(p.m_a, l, name2char(id), args->arguments, \
        STMTS(stmts), stmts.size(), nullptr, 0, EXPR(return),\
        extract_type_comment(p, l, type_comment))

#define ASYNC_FOR_01(target, iter, stmts, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), nullptr, 0, nullptr)
#define ASYNC_FOR_02(target, iter, stmts, orelse, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), STMTS(orelse), orelse.size(), nullptr)
#define ASYNC_FOR_03(target, iter, stmts, type_comment, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), nullptr, 0, \
        extract_type_comment(p, l, type_comment))
#define ASYNC_FOR_04(target, iter, stmts, orelse, type_comment, l) make_AsyncFor_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(SET_CTX_01(target, Store), Store)), EXPR(iter), \
        STMTS(stmts), stmts.size(), STMTS(orelse), orelse.size(), \
        extract_type_comment(p, l, type_comment))

#define ASYNC_WITH_01(items, body, l) make_AsyncWith_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), nullptr)
#define ASYNC_WITH_02(items, body, type_comment, l) make_AsyncWith_t(p.m_a, l, \
        items.p, items.size(), STMTS(body), body.size(), \
        extract_type_comment(p, l, type_comment))

#define WHILE_01(e, stmts, l) make_While_t(p.m_a, l, \
        EXPR(e), STMTS(stmts), stmts.size(), nullptr, 0)
#define WHILE_02(e, stmts, orelse, l) make_While_t(p.m_a, l, \
        EXPR(e), STMTS(stmts), stmts.size(), STMTS(orelse), orelse.size())

static inline ast_t* BOOLOP_01(Allocator &al, Location &loc,
        boolopType op, ast_t *x, ast_t *y) {
    expr_t* x1 = EXPR(x);
    expr_t* y1 = EXPR(y);
    Vec<expr_t*> v;
    v.reserve(al, 4);
    /*
    if (is_a<BoolOp_t>(*x1)) {
        BoolOp_t* tmp = down_cast<BoolOp_t>(x1);
        if (op == tmp->m_op) {
            for(size_t i = 0; i < tmp->n_values; i++) {
                v.push_back(al, tmp->m_values[i]);
            }
            v.push_back(al, y1);
        } else {
            v.push_back(al, x1);
            v.push_back(al, y1);
        }
    */
    v.push_back(al, x1);
    v.push_back(al, y1);
    return make_BoolOp_t(al, loc, op, v.p, v.n);
}

#define BOOLOP(x, op, y, l) BOOLOP_01(p.m_a, l, op, x, y)
#define BINOP(x, op, y, l) make_BinOp_t(p.m_a, l, \
        EXPR(x), operatorType::op, EXPR(y))
#define UNARY(x, op, l) make_UnaryOp_t(p.m_a, l, unaryopType::op, EXPR(x))
#define COMPARE(x, op, y, l) make_Compare_t(p.m_a, l, \
        EXPR(x), cmpopType::op, EXPRS(A2LIST(p.m_a, y)), 1)

static inline ast_t* concat_string(Allocator &al, Location &l,
        expr_t *string, std::string str, expr_t *string_literal) {
    std::string str1 = "";
    ast_t* tmp = nullptr;
    Vec<expr_t *> exprs;
    exprs.reserve(al, 4);

    // TODO: Merge two concurrent ConstantStr's into one in the JoinedStr
    if (string_literal) {
        if (is_a<ConstantStr_t>(*string)
                && is_a<ConstantStr_t>(*string_literal)) {
            str1 = std::string(down_cast<ConstantStr_t>(string)->m_value);
            str = std::string(down_cast<ConstantStr_t>(string_literal)->m_value);
            str1 = str1 + str;
            tmp = make_ConstantStr_t(al, l, LCompilers::s2c(al, str1), nullptr);
        } else if (is_a<JoinedStr_t>(*string)
                && is_a<JoinedStr_t>(*string_literal)) {
            JoinedStr_t *t = down_cast<JoinedStr_t>(string);
            for (size_t i = 0; i < t->n_values; i++) {
                exprs.push_back(al, t->m_values[i]);
            }
            t = down_cast<JoinedStr_t>(string_literal);
            for (size_t i = 0; i < t->n_values; i++) {
                exprs.push_back(al, t->m_values[i]);
            }
            tmp = make_JoinedStr_t(al, l, exprs.p, exprs.size());
        } else if (is_a<JoinedStr_t>(*string)
                && is_a<ConstantStr_t>(*string_literal)) {
            JoinedStr_t *t = down_cast<JoinedStr_t>(string);
            for (size_t i = 0; i < t->n_values; i++) {
                exprs.push_back(al, t->m_values[i]);
            }
            exprs.push_back(al, string_literal);
            tmp = make_JoinedStr_t(al, l, exprs.p, exprs.size());
        } else if (is_a<ConstantStr_t>(*string)
                && is_a<JoinedStr_t>(*string_literal)) {
            exprs.push_back(al, string);
            JoinedStr_t *t = down_cast<JoinedStr_t>(string_literal);
            for (size_t i = 0; i < t->n_values; i++) {
                exprs.push_back(al, t->m_values[i]);
            }
            tmp = make_JoinedStr_t(al, l, exprs.p, exprs.size());
        } else if (is_a<ConstantBytes_t>(*string)
                && is_a<ConstantBytes_t>(*string_literal)) {
            str1 = std::string(down_cast<ConstantBytes_t>(string)->m_value);
            str1 = str1.substr(0, str1.size() - 1);
            str = std::string(down_cast<ConstantBytes_t>(string_literal)->m_value);
            str = str.substr(2, str.size());
            str1 = str1 + str;
            tmp = make_ConstantBytes_t(al, l, LCompilers::s2c(al, str1), nullptr);
        } else {
            throw LCompilers::LPython::parser_local::ParserError(
                "The byte and non-byte literals can not be combined", l);
        }
    } else {
        if (is_a<ConstantStr_t>(*string)) {
            str1 = std::string(down_cast<ConstantStr_t>(string)->m_value);
            str1 = str1 + str;
            tmp = make_ConstantStr_t(al, l, LCompilers::s2c(al, str1), nullptr);
        } else if (is_a<JoinedStr_t>(*string)) {
            JoinedStr_t *t = down_cast<JoinedStr_t>(string);
            for (size_t i = 0; i < t->n_values; i++) {
                exprs.push_back(al, t->m_values[i]);
            }
            exprs.push_back(al, (expr_t *)make_ConstantStr_t(al, l,
                            LCompilers::s2c(al, str), nullptr));
            tmp = make_JoinedStr_t(al, l, exprs.p, exprs.size());
        } else {
            LCOMPILERS_ASSERT(false);
        }
    }
    return tmp;
}

#define SYMBOL(x, l) make_Name_t(p.m_a, l, \
        x.c_str(p.m_a), expr_contextType::Load)
// `x.int_n` is of type BigInt but we store the int64_t directly in AST
#define INTEGER(x, l) make_ConstantInt_t(p.m_a, l, x, nullptr)
#define STRING1(x, l) make_ConstantStr_t(p.m_a, l, x.c_str(p.m_a), nullptr)
#define STRING2(x, l) make_ConstantStr_t(p.m_a, l, x.c_str(p.m_a), LCompilers::s2c(p.m_a, "u"))
#define STRING3(x, l) FMT_STRING(p.m_a, l, x.c_str(p.m_a))
#define STRING4(x, y, l) concat_string(p.m_a, l, EXPR(x), y.c_str(p.m_a), nullptr)
#define STRING5(x, s, l) concat_string(p.m_a, l, EXPR(x), "", EXPR(s))
#define BYTES1(x, l) make_ConstantBytes_t(p.m_a, l, x.c_str(p.m_a), nullptr)
#define FLOAT(x, l) make_ConstantFloat_t(p.m_a, l, x, nullptr)
#define COMPLEX(x, l) make_ConstantComplex_t(p.m_a, l, 0, x, nullptr)
#define BOOL(x, l) make_ConstantBool_t(p.m_a, l, x, nullptr)

static inline ast_t *FMT_STRING(Allocator &al, Location &l, char *s){
    Vec<expr_t *> exprs;
    exprs.reserve(al, 4);
    ast_t *tmp = nullptr;

    std::string str = std::string(s);
    std::string s1 = "\"";
    std::string id;
    std::vector<std::string> strs;
    bool open_paren = false;
    for (size_t i = 0; i < str.length(); i++) {
            if(str[i] == '{') {
            if(s1 != "\"") {
                    s1.push_back('"');
                    strs.push_back(s1);
                    s1 = "\"";
            }
            open_paren = true;
            } else if (str[i] != '}' && open_paren) {
            id.push_back(s[i]);
            } else if (str[i] == '}') {
            if(id != "") {
                    strs.push_back(id);
                    id = "";
            }
            open_paren = false;
            } else if (!open_paren) {
            s1.push_back(s[i]);
            }
            if(i == str.length()-1 && s1 != "\"") {
            s1.push_back('"');
            strs.push_back(s1);
            }
    }

    for (size_t i = 0; i < strs.size(); i++) {
            if (strs[i][0] == '"') {
            strs[i] = strs[i].substr(1, strs[i].length() - 2);
            tmp = make_ConstantStr_t(al, l, LCompilers::s2c(al, strs[i]), nullptr);
            exprs.push_back(al, down_cast<expr_t>(tmp));
            } else {
            tmp = make_Name_t(al, l,
                    LCompilers::s2c(al, strs[i]), expr_contextType::Load);
            tmp = make_FormattedValue_t(al, l, EXPR(tmp), -1, nullptr);
            exprs.push_back(al, down_cast<expr_t>(tmp));
            }
    }

    return make_JoinedStr_t(al, l, exprs.p, exprs.size());
}

static inline keyword_t *CALL_ARG_KW(Allocator &al, Location &l,
        char *arg, expr_t* val) {
    keyword_t *r = al.allocate<keyword_t>();
    r->loc = l;
    r->m_arg = arg;
    r->m_value = val;
    return r;
}
#define CALL_KW_01(arg, val, l) CALL_ARG_KW(p.m_a, l, \
        name2char(arg), EXPR(val))
#define CALL_EXPR_01(val, l) CALL_ARG_KW(p.m_a, l, nullptr, EXPR(val))

static inline Kw_or_Star_Arg *KW_OR_EXPR(Allocator &al,
        expr_t *expr, keyword_t *kw_arg) {
    Kw_or_Star_Arg *r = al.allocate<Kw_or_Star_Arg>();
    r->star_arg = expr;
    r->kw_arg = kw_arg;
    return r;
}
#define CALL_KW_02(id, e, l) KW_OR_EXPR(p.m_a, nullptr, CALL_KW_01(id, e, l))
#define CALL_EXPR_02(e, l) KW_OR_EXPR(p.m_a, EXPR(e), nullptr)

static inline Call_Arg *CALL_ARG(
        Allocator &al,
        Kw_or_Star_Arg *m_expr_or_kw, size_t n_expr_or_kw,
        keyword_t *m_kw, size_t n_kw
    ) {
    Call_Arg *r = al.allocate<Call_Arg>();
    Vec<expr_t*> exprs;
    exprs.reserve(al, n_expr_or_kw);
    Vec<keyword_t> kws;
    kws.reserve(al, n_kw);

    for (size_t i=0; i<n_expr_or_kw; i++) {
        if (m_expr_or_kw[i].star_arg != nullptr) {
            exprs.push_back(al, m_expr_or_kw[i].star_arg);
        } else if (m_expr_or_kw[i].kw_arg != nullptr) {
            kws.push_back(al, *m_expr_or_kw[i].kw_arg);
        }
    }
    for (size_t i=0; i<n_kw; i++) {
        kws.push_back(al, m_kw[i]);
    }

    r->expr.p = exprs.p;
    r->expr.n = exprs.n;
    r->kw.p = kws.p;
    r->kw.n = kws.n;
    return r;
}

#define CALL_ARG_00() CALL_ARG(p.m_a, nullptr, 0, nullptr, 0)
#define CALL_ARG_01(exprs) CALL_ARG(p.m_a, exprs.p, exprs.n, nullptr, 0)
#define CALL_ARG_02(kwargs) CALL_ARG(p.m_a, nullptr, 0, kwargs.p, kwargs.n)
#define CALL_ARG_03(exprs, kwargs) CALL_ARG(p.m_a, exprs.p, exprs.n, \
        kwargs.p, kwargs.n)

#define CALL_01(func, args, l) make_Call_t(p.m_a, l, \
        EXPR(func), args->expr.p, args->expr.n, args->kw.p, args->kw.n)
#define CALL_02(func, comp, l) make_Call_t(p.m_a, l, \
        EXPR(func), EXPRS(comp), comp.size(), nullptr, 0)

static inline comprehension_t *COMP(Allocator &al, Location &l,
        expr_t *target, expr_t* iter, expr_t **ifs, size_t ifs_size,
        int64_t is_async) {
    comprehension_t *r = al.allocate<comprehension_t>();
    r->loc = l;
    r->m_target = target;
    r->m_iter = iter;
    r->m_ifs = ifs;
    r->n_ifs = ifs_size;
    r->m_is_async = is_async;
    return r;
}
#define COMP_FOR_01(target, iter, l) COMP(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(target, Store)), EXPR(iter), nullptr, 0, 0)
#define COMP_FOR_02(target, iter, ifs, l) COMP(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(target, Store)), EXPR(iter), \
        EXPRS(ifs), ifs.size(), 0)
#define COMP_FOR_03(target, iter, l) COMP(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(target, Store)), EXPR(iter), nullptr, 0, 1)
#define COMP_FOR_04(target, iter, ifs, l) COMP(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(target, Store)), EXPR(iter), \
        EXPRS(ifs), ifs.size(), 1)

#define GENERATOR_EXPR(elt, generators, l) make_GeneratorExp_t(p.m_a, l, \
        EXPR(elt), generators.p, generators.n)

#define LIST(e, l) make_List_t(p.m_a, l, \
        EXPRS(e), e.size(), expr_contextType::Load)
#define ATTRIBUTE_REF(val, attr, l) make_Attribute_t(p.m_a, l, \
        EXPR(val), name2char(attr), expr_contextType::Load)

static inline ast_t* ID_TUPLE_02(Allocator &al, Location &l, Vec<ast_t*> elts) {
    if(is_a<expr_t>(*elts[0]) && elts.size() == 1) {
        return (ast_t*) SET_EXPR_CTX_01(elts[0], Store);
    }
    return make_Tuple_t(al, l, EXPRS(SET_EXPR_CTX_02(SET_CTX_02(elts, Store), Store)),
                                elts.size(), expr_contextType::Store);
}
#define ID_TUPLE_01(elts, l) ID_TUPLE_02(p.m_a, l, elts)
#define ID_TUPLE_03(elts, l) make_Tuple_t(p.m_a, l, \
        EXPRS(SET_EXPR_CTX_02(SET_CTX_02(elts, Store), Store)), elts.size(), \
        expr_contextType::Store)

#define LIST_COMP_1(expr, generators, l) make_ListComp_t(p.m_a, l, \
        EXPR(expr), generators.p, generators.n)
#define SET_COMP_1(expr, generators, l) make_SetComp_t(p.m_a, l, \
        EXPR(expr), generators.p, generators.n)
#define DICT_COMP_1(key, val, generators, l) make_DictComp_t(p.m_a, l, \
        EXPR(key), EXPR(val), generators.p, generators.n)
#define COMP_EXPR_1(expr, generators, l) make_GeneratorExp_t(p.m_a, l, \
        EXPR(expr), generators.p, generators.n)

#define ELLIPSIS(l) make_ConstantEllipsis_t(p.m_a, l, nullptr)

#define NONE(l) make_ConstantNone_t(p.m_a, l, nullptr)

#define SUBSCRIPT_01(value, slice, l) make_Subscript_t(p.m_a, l, \
        EXPR(value), EXPR(slice), expr_contextType::Load)

static inline ast_t* SLICE(Allocator &al, Location &l,
        ast_t *lower, ast_t *upper, ast_t *_step) {
    expr_t* start = EXPR_OPT(lower);
    expr_t* end = EXPR_OPT(upper);
    expr_t* step = EXPR_OPT(_step);
    return make_Slice_t(al, l, start, end, step);
}

#define SLICE_01(lower, upper, step, l) SLICE(p.m_a, l, lower, upper, step)

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

#define DICT_EXPR_01(key, value, l) DICT(p.m_a, EXPR(key), EXPR(value))
#define DICT_EXPR_02(key, type_comment, value, l) DICT(p.m_a, EXPR(key), \
        EXPR(value)); extract_type_comment(p, l, type_comment)
#define DICT_01(l) make_Dict_t(p.m_a, l, nullptr, 0, nullptr, 0)
#define DICT_02(dict_list, l) DICT1(p.m_a, l, dict_list)
#define AWAIT(e, l) make_Await_t(p.m_a, l, EXPR(e))
#define SET(e, l) make_Set_t(p.m_a, l, EXPRS(e), e.size())
#define NAMEDEXPR(x, y, l) make_NamedExpr_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(x, Store)), EXPR(y))
#define STARRED_ARG(e, l) make_Starred_t(p.m_a, l, \
        EXPR(e), expr_contextType::Load)
#define STARRED_ARG_STORE(e, l) make_Starred_t(p.m_a, l, \
        EXPR(SET_EXPR_CTX_01(e, Store)), expr_contextType::Load)
#define LAMBDA_01(args, e, l) make_Lambda_t(p.m_a, l, args->arguments, EXPR(e))

#endif
