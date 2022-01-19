#ifndef LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H
#define LFORTRAN_SEMANTICS_SEMANTIC_EXCEPTION_H

#include <libasr/exception.h>

namespace LFortran {

// This exception is only used internally in the lfortran/semantics/ directory
// and in lfortran/asr_utils.h/cpp. Nowhere else.

class SemanticError
{
public:
    diag::Diagnostic d;
public:
    SemanticError(const std::string &msg, const Location &loc)
        : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::Semantic, {
            diag::Label("", {loc})
        })}
    { }

    SemanticError(const diag::Diagnostic &d) : d{d} { }
};

class SemanticAbort
{
};

}


#endif
