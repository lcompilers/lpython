#ifndef LPYTHON_PARSER_STYPE_H
#define LPYTHON_PARSER_STYPE_H

#include <cstring>
#include <lpython/python_ast.h>
#include <libasr/location.h>
#include <libasr/containers.h>
#include <libasr/bigint.h>

namespace LCompilers::LPython {

struct Key_Val {
    LPython::AST::expr_t* key;
    LPython::AST::expr_t* value;
};

struct Args {
    LPython::AST::arguments_t arguments;
};

struct Arg {
    bool default_value;
    LPython::AST::arg_t _arg;
    LPython::AST::expr_t *defaults;
};

struct Var_Kw {
    Vec<Arg*> vararg;
    Vec<Arg*> kwonlyargs;
    Vec<Arg*> kwarg;
};

struct Args_ {
    Vec<Arg*> args;
    bool var_kw_val;
    Var_Kw *var_kw;
};

struct Fn_Arg {
    Vec<Arg*> posonlyargs;
    bool args_val;
    Args_ *args;
};

struct Kw_or_Star_Arg {
    LPython::AST::expr_t *star_arg;
    LPython::AST::keyword_t *kw_arg;
};

struct Call_Arg {
    Vec<LPython::AST::expr_t*> expr;
    Vec<LPython::AST::keyword_t> kw;
};

struct Key_Val_Pattern {
    LPython::AST::expr_t* key;
    LPython::AST::pattern_t* val;
};

union YYSTYPE {
    int64_t n;
    double f;
    Str string;

    LPython::AST::ast_t* ast;
    Vec<LPython::AST::ast_t*> vec_ast;

    LPython::AST::alias_t* alias;
    Vec<LPython::AST::alias_t> vec_alias;

    Arg *arg;
    Vec<Arg*> vec_arg;

    Args *args;
    Vec<Args> vec_args;

    Fn_Arg *fn_arg;
    Args_ *args_;
    Var_Kw *var_kw;

    Key_Val *key_val;
    Vec<Key_Val*> vec_key_val;

    LPython::AST::withitem_t* withitem;
    Vec<LPython::AST::withitem_t> vec_withitem;

    LPython::AST::keyword_t* keyword;
    Vec<LPython::AST::keyword_t> vec_keyword;

    Kw_or_Star_Arg* kw_or_star;
    Vec<Kw_or_Star_Arg> vec_kw_or_star;

    Call_Arg *call_arg;

    LPython::AST::comprehension_t* comp;
    Vec<LPython::AST::comprehension_t> vec_comp;

    LPython::AST::operatorType operator_type;

    LPython::AST::match_case_t* match_case;
    Vec<LPython::AST::match_case_t> vec_match_case;

    LPython::AST::pattern_t* pattern;
    Vec<LPython::AST::pattern_t*> vec_pattern;

    Key_Val_Pattern *kw_val_pattern;
    Vec<Key_Val_Pattern> vec_kw_val_pattern;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);
// Ensure the YYSTYPE size is equal to Vec<AST::ast_t*>, which is a required member, so
// YYSTYPE must be at least as big, but it should not be bigger, otherwise it
// would reduce performance.
// A temporary fix for PowerPC 32-bit, where the following assert fails with (16 == 12).
#ifndef __ppc__
static_assert(sizeof(YYSTYPE) == sizeof(Vec<LPython::AST::ast_t*>));
#endif

static_assert(std::is_standard_layout<Location>::value);
static_assert(std::is_trivial<Location>::value);

} // namespace LCompilers::LPython


typedef struct LCompilers::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 0
#define YYINITDEPTH 2000


#endif // LPYTHON_PARSER_STYPE_H
