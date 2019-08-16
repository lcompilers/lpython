#ifndef LFORTRAN_PARSER_STYPE_H
#define LFORTRAN_PARSER_STYPE_H

#include <lfortran/ast.h>

namespace LFortran
{

struct YYSTYPE {
    LFortran::AST::expr_t* basic;
    unsigned long n;
    std::string string;
    // Constructor
    YYSTYPE() = default;
    // Destructor
    ~YYSTYPE() = default;
    // Copy constructor and assignment
    YYSTYPE(const YYSTYPE &) = default;
    YYSTYPE &operator=(const YYSTYPE &) = default;
    // Move constructor and assignment
    YYSTYPE(YYSTYPE &&) = default;
    YYSTYPE &operator=(YYSTYPE &&) = default;
};

} // namespace LFortran



#endif
