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

union YYSTYPE {
    unsigned long n;
    LFortran::AST::ast_t* ast;
    using Vec = LFortran::Vec<LFortran::AST::ast_t*>;
    using VecDecl = LFortran::Vec<LFortran::AST::decl_t>;
    Vec vec_ast;
    VecDecl vec_decl;

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
