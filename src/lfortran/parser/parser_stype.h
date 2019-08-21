#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <vector>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>

namespace LFortran
{

struct YYSTYPE {
    LFortran::AST::ast_t* ast;
    struct Vec {size_t n; LFortran::AST::ast_t** p;} vec_ast;
    unsigned long n;
    struct Str {size_t n; char* p;} string; // Not null-terminated
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1


#endif
