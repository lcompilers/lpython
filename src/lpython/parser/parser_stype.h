#ifndef LPYTHON_PARSER_STYPE_H
#define LPYTHON_PARSER_STYPE_H

#include <cstring>
#include <lpython/python_ast.h>
#include <libasr/location.h>
#include <libasr/containers.h>
#include <libasr/bigint.h>

namespace LFortran
{

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

    Key_Val *key_val;
    Vec<Key_Val*> vec_key_val;

    LPython::AST::withitem_t* withitem;
    Vec<LPython::AST::withitem_t> vec_withitem;

    LPython::AST::keyword_t* keyword;
    Vec<LPython::AST::keyword_t> vec_keyword;

    LPython::AST::comprehension_t* comp;
    Vec<LPython::AST::comprehension_t> vec_comp;

    LPython::AST::operatorType operator_type;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);
// Ensure the YYSTYPE size is equal to Vec<AST::ast_t*>, which is a required member, so
// YYSTYPE has to be at least as big, but it should not be bigger, otherwise it
// would reduce performance.
static_assert(sizeof(YYSTYPE) == sizeof(Vec<LPython::AST::ast_t*>));

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 0


#endif // LPYTHON_PARSER_STYPE_H
