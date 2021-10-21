#ifndef LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H
#define LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H

#include <lfortran/exception.h>

namespace LFortran {

class SemanticError : public LFortranException
{
public:
    diag::Diagnostic d;
public:
    SemanticError(const std::string &msg, const Location &loc)
        : LFortranException(msg, LFORTRAN_SEMANTIC_ERROR),
        d{diag::Diagnostic::semantic_error(msg, loc)}
    { }

    SemanticError(const diag::Diagnostic &d)
            : LFortranException(d.message, LFORTRAN_SEMANTIC_ERROR),
            d{d} {
    }
};

}


#endif
