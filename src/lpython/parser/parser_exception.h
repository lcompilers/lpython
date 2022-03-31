#ifndef LPYTHON_PARSER_PARSER_EXCEPTION_H
#define LPYTHON_PARSER_PARSER_EXCEPTION_H

#include <libasr/exception.h>
#include <libasr/diagnostics.h>

namespace LFortran {

namespace parser_local {

    // Local exceptions that are used to terminate the tokenizer.

    class TokenizerError
    {
    public:
        diag::Diagnostic d;
    public:
        TokenizerError(const std::string &msg, const Location &loc)
            : d{diag::Diagnostic(msg, diag::Level::Error, diag::Stage::Tokenizer, {
                diag::Label("", {loc})
            })}
        { }

        TokenizerError(const diag::Diagnostic &d) : d{d} { }
    };

}

}


#endif // LPYTHON_PARSER_PARSER_EXCEPTION_H
