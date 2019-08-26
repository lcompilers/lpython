#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <cstring>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>

namespace LFortran
{

// Vector implementation
struct Vec {
    size_t n, max;
    LFortran::AST::ast_t** p;
    // reserve() must be called before calling push_back()
    void reserve(Allocator &al, size_t max) {
        n = 0;
        this->max = max;
        p = al.allocate<LFortran::AST::ast_t*>(max);
    }
    void push_back(Allocator &al, LFortran::AST::ast_t *x) {
        if (n == max) {
            size_t max2 = 2*max;
            LFortran::AST::ast_t** p2 = al.allocate<LFortran::AST::ast_t*>(max2);
            std::memcpy(p2, p, sizeof(LFortran::AST::ast_t*) * max);
            p = p2;
            max = max2;
        }
        p[n] = x;
        n++;
    }
    size_t size() const {
        return n;
    }
};

union YYSTYPE {
    unsigned long n;
    LFortran::AST::ast_t* ast;
    using Vec = LFortran::Vec;
    Vec vec_ast;

    // String implementation (not null-terminated)
    struct Str {
        size_t n;
        char* p;
        // Returns a copy of the string as a NULL terminated std::string
        std::string str() const { return std::string(p, n); }
        // Returns a copy of the string as a NULL terminated C string,
        // allocated using Allocator
        char* c_str(Allocator &al) const {
            char *s = al.allocate<char>(n+1);
            std::memcpy(s, p, sizeof(char) * n);
            s[n] = '\0';
            return s;
        }
        size_t size() const {
            return n;
        }
    } string;
};

static_assert(std::is_standard_layout<YYSTYPE>::value);
static_assert(std::is_trivial<YYSTYPE>::value);

} // namespace LFortran


typedef struct LFortran::Location YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1


#endif
