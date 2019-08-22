#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <cstring>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>

namespace LFortran
{

union YYSTYPE {
    unsigned long n;
    LFortran::AST::ast_t* ast;

    // Vector implementation
    struct Vec {
        size_t n, max;
        LFortran::AST::ast_t** p;
        void push_back(Allocator &al, LFortran::AST::ast_t *x) {
            if (n == max) {
                size_t max2 = 2*max;
                LFortran::AST::ast_t** p2 = (LFortran::AST::ast_t**)
                    al.allocate(sizeof(LFortran::AST::ast_t*) * max2);
                std::memcpy(p2, p, sizeof(LFortran::AST::ast_t*) * max);
                p = p2;
                max = max2;
            }
            p[n] = x;
            n++;
        }
    } vec_ast;

    // String implementation (not null-terminated)
    struct Str {
        size_t n;
        char* p;
        std::string str() const { return std::string(p, n); }
    } string;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1


#endif
