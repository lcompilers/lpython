#ifndef LPYTHON_SEMANTICS_SEMANTIC_EXCEPTION_H
#define LPYTHON_SEMANTICS_SEMANTIC_EXCEPTION_H

#include <libasr/exception.h>

namespace LCompilers::LPython {

// This exception is only used internally in the lpython/semantics/ directory
// and in lpython/asr_utils.h/cpp. Nowhere else.

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

} // namespace LCompilers::LPython


#endif
