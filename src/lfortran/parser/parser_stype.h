#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <cstring>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>
#include <lfortran/containers.h>

namespace LFortran
{

struct VarType {
    Location loc;
    Str string;
    Vec<AST::kind_item_t> kind;
    char *identifier;
};

struct FnArg {
    bool keyword;
    union {
        AST::fnarg_t arg;
        AST::keyword_t kw;
    };
};

struct ArgStarKw {
    bool keyword;
    union {
        AST::argstar_t arg;
        AST::kw_argstar_t kw;
    };
};

union YYSTYPE {
    unsigned long n;
    Str string;

    AST::ast_t* ast;
    Vec<AST::ast_t*> vec_ast;

    AST::var_sym_t *var_sym;
    Vec<AST::var_sym_t> vec_var_sym;

    AST::dimension_t *dim;
    Vec<AST::dimension_t> vec_dim;

    AST::codimension_t *codim;
    Vec<AST::codimension_t> vec_codim;

    AST::reduce_opType reduce_op_type;

    VarType *var_type;

    AST::kind_item_t *kind_arg;
    Vec<AST::kind_item_t> vec_kind_arg;

    FnArg *fnarg;
    Vec<FnArg> vec_fnarg;

    ArgStarKw *argstarkw;
    Vec<ArgStarKw> vec_argstarkw;

    AST::struct_member_t *struct_member;
    Vec<AST::struct_member_t> vec_struct_member;

    AST::interfaceopType interface_op_type;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);
// Ensure the YYSTYPE size is equal to Vec<AST::ast_t*>, which is a required member, so
// YYSTYPE has to be at least as big, but it should not be bigger, otherwise it
// would reduce performance.
static_assert(sizeof(YYSTYPE) == sizeof(Vec<AST::ast_t*>));

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1


#endif
