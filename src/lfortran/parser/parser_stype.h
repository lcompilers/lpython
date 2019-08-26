#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <cstring>
#include <lfortran/ast.h>
#include <lfortran/parser/location.h>

namespace LFortran
{

// Vector implementation
template <typename T>
struct Vec {
    size_t n, max;
    T* p;
    // reserve() must be called before calling push_back()
    void reserve(Allocator &al, size_t max) {
        n = 0;
        this->max = max;
        p = al.allocate<T>(max);
    }
    void push_back(Allocator &al, T x) {
        if (n == max) {
            size_t max2 = 2*max;
            T* p2 = al.allocate<T>(max2);
            std::memcpy(p2, p, sizeof(T) * max);
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
};

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
