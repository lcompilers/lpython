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
#include <lfortran/diagnostics.h>

namespace LFortran
{

struct Error {
    std::vector<StacktraceItem> stacktrace_addresses;
    diag::Diagnostic d;
};

template<typename T>
struct Result {
    bool ok;
    union {
        T result;
        Error error;
    };
    // Default constructor
    Result() = delete;
    // Success result constructor
    Result(const T &result) : ok{true}, result{result} {}
    // Error result constructor
    Result(const Error &error) : ok{false}, error{error} {}
    // Destructor
    ~Result() {
        if (!ok) {
            error.~Error();
        }
    }
    // Copy constructor
    Result(const Result<T> &other) : ok{other.ok} {
        if (ok) {
            new(&result) T(other.result);
        } else {
            new(&error) Error(other.error);
        }
    }
    // Copy assignment
    Result<T>& operator=(const Result<T> &other) {
        ok = other.ok;
        if (ok) {
            new(&result) T(other.result);
        } else {
            new(&error) Error(other.error);
        }
        return *this;
    }
    // Move constructor
    Result(T &&result) : ok{true}, result{std::move(result)} {}
    // Move assignment
    Result<T>&& operator=(T &&other) = delete;
};


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

template<typename T>
static inline T TRY(Result<T> result) {
    if (result.ok) {
        return result.result;
    } else {
        throw LFortranException("Internal Compiler Error: TRY failed, error was not handled explicitly");
    }
}


} // Namespace LFortran

#endif // __cplusplus
#endif // LFORTRAN_EXCEPTION_H
