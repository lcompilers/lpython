#ifndef LFORTRAN_PARSER_PARSER_EXCEPTION_H
#define LFORTRAN_PARSER_PARSER_EXCEPTION_H

namespace {

    // Local exception that is used to terminate the parser.
    // It is not propagated outside.
    // This file is included in parser.tab.cc (via semantics.h)
    // And in parser.cpp. Nowhere else.
    class ParserError : public LFortran::LFortranException
    {
    public:
        LFortran::diag::Diagnostic d;
    public:
        ParserError(const std::string &msg, const LFortran::Location &loc)
            : LFortranException(msg, LFORTRAN_PARSER_ERROR),
            d{LFortran::diag::Diagnostic::parser_error(msg, loc)}
        {
        }

        ParserError(const std::string &msg)
            : LFortranException(msg, LFORTRAN_PARSER_ERROR),
            d{LFortran::diag::Diagnostic::parser_error(msg)}
        {
        }
    };

}


#endif
