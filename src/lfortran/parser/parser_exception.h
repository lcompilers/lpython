#ifndef LFORTRAN_PARSER_PARSER_EXCEPTION_H
#define LFORTRAN_PARSER_PARSER_EXCEPTION_H

#include <lfortran/exception.h>

namespace LFortran {

namespace parser_local {

    // Local exceptions that are used to terminate the parser.
    // It is not propagated outside.
    // This file is included in parser.tab.cc (via semantics.h)
    // And in parser.cpp. Nowhere else.

    class TokenizerError
    {
    public:
        diag::Diagnostic d;
        std::vector<StacktraceItem> m_stacktrace_addresses;
    public:
        TokenizerError(const std::string &msg, const Location &loc)
            : d{diag::Diagnostic::tokenizer_error(msg, loc)},
            m_stacktrace_addresses{get_stacktrace_addresses()}
        { }

        TokenizerError(const diag::Diagnostic &d)
             : d{d},
            m_stacktrace_addresses{get_stacktrace_addresses()}
        { }
    };

    class ParserError
    {
    public:
        LFortran::diag::Diagnostic d;
        std::vector<StacktraceItem> m_stacktrace_addresses;
    public:
        ParserError(const std::string &msg, const LFortran::Location &loc)
            : d{LFortran::diag::Diagnostic::parser_error(msg, loc)},
            m_stacktrace_addresses{get_stacktrace_addresses()}
        {
        }

        ParserError(const std::string &msg)
            : d{LFortran::diag::Diagnostic::parser_error(msg)},
            m_stacktrace_addresses{get_stacktrace_addresses()}
        {
        }
    };

}
}


#endif
