#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <cstring>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>
#include <lfortran/containers.h>

namespace LFortran
{

union YYSTYPE {
    using Str = LFortran::Str;
    using VecAST = LFortran::Vec<LFortran::AST::ast_t*>;
    using VecDecl = LFortran::Vec<LFortran::AST::decl_t>;
    using VecDim = LFortran::Vec<LFortran::AST::dimension_t>;

    unsigned long n;
    Str string;

    LFortran::AST::ast_t* ast;
    VecAST vec_ast;

    LFortran::AST::decl_t *decl; // Pointer, to reduce size of YYSTYPE
    VecDecl vec_decl;

    LFortran::AST::dimension_t dim;
    VecDim vec_dim;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);
// Ensure the YYSTYPE size is equal to VecAST, which is a required member, so
// YYSTYPE has to be at least as big, but it should not be bigger, otherwise it
// would reduce performance.
static_assert(sizeof(YYSTYPE) == sizeof(YYSTYPE::VecAST));

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1


#endif
