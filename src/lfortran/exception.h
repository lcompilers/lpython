#ifndef LFORTRAN_EXCEPTION_H
#define LFORTRAN_EXCEPTION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LFORTRAN_NO_EXCEPTION    = 0,
    LFORTRAN_RUNTIME_ERROR   = 1,
    LFORTRAN_EXCEPTION       = 2,
    LFORTRAN_TOKENIZER_ERROR = 3,
    LFORTRAN_PARSER_ERROR    = 4,
    LFORTRAN_SEMANTIC_ERROR  = 5,
    LFORTRAN_CODEGEN_ERROR   = 6,
    LFORTRAN_ASSERT_FAILED   = 7,
    LFORTRAN_ASSEMBLER_ERROR = 8,
} lfortran_exceptions_t;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <exception>
#include <string>
#include <lfortran/parser/location.h>
#include <lfortran/config.h>
#include <lfortran/stacktrace.h>

namespace LFortran
{

const int stacktrace_depth = 4;

class LFortranException : public std::exception
{
    std::string m_msg;
    lfortran_exceptions_t ec;
    std::vector<StacktraceItem> m_stacktrace_addresses;

public:
    LFortranException(const std::string &msg, lfortran_exceptions_t error)
        : m_msg{msg}, ec{error}
    {
#if defined(HAVE_LFORTRAN_STACKTRACE)
        m_stacktrace_addresses = get_stacktrace_addresses();
#endif
    }
    LFortranException(const std::string &msg)
        : LFortranException(msg, LFORTRAN_EXCEPTION)
    {
    }
    const char *what() const throw()
    {
        return m_msg.c_str();
    }
    std::string msg() const
    {
        return m_msg;
    }
    std::string name() const
    {
        switch (ec) {
            case (lfortran_exceptions_t::LFORTRAN_EXCEPTION) :
                return "LFortranException";
            case (lfortran_exceptions_t::LFORTRAN_TOKENIZER_ERROR) :
                return "TokenizerError";
            case (lfortran_exceptions_t::LFORTRAN_PARSER_ERROR) :
                return "ParserError";
            case (lfortran_exceptions_t::LFORTRAN_SEMANTIC_ERROR) :
                return "SemanticError";
            case (lfortran_exceptions_t::LFORTRAN_CODEGEN_ERROR) :
                return "CodeGenError";
            case (lfortran_exceptions_t::LFORTRAN_ASSERT_FAILED) :
                return "AssertFailed";
            default : return "Unknown Exception";
        }
    }
    std::vector<StacktraceItem> stacktrace_addresses() const
    {
        return m_stacktrace_addresses;
    }
    lfortran_exceptions_t error_code()
    {
        return ec;
    }
};

class TokenizerError : public LFortranException
{
public:
    Location loc;
    std::string token;
public:
    TokenizerError(const std::string &msg, const Location &loc,
            const std::string &token)
        : LFortranException(msg, LFORTRAN_TOKENIZER_ERROR), loc{loc},
            token{token}
    {
    }
};

class ParserError : public LFortranException
{
public:
    Location loc;
    int token;
public:
    ParserError(const std::string &msg, const Location &loc, const int token)
        : LFortranException(msg, LFORTRAN_PARSER_ERROR), loc{loc}, token{token}
    {
    }
};

class SemanticError : public LFortranException
{
public:
    Location loc;
public:
    SemanticError(const std::string &msg, const Location &loc)
        : LFortranException(msg, LFORTRAN_SEMANTIC_ERROR), loc{loc}
    {
    }
};

class CodeGenError : public LFortranException
{
public:
    CodeGenError(const std::string &msg)
        : LFortranException(msg, LFORTRAN_CODEGEN_ERROR)
    {
    }
};

class AssertFailed : public LFortranException
{
public:
    AssertFailed(const std::string &msg)
        : LFortranException(msg, LFORTRAN_ASSERT_FAILED)
    {
    }
};

class AssemblerError : public LFortranException
{
public:
    AssemblerError(const std::string &msg)
        : LFortranException(msg, LFORTRAN_ASSEMBLER_ERROR)
    {
    }
};

} // Namespace LFortran

#endif // __cplusplus
#endif // LFORTRAN_EXCEPTION_H
