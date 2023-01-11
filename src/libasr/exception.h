#ifndef LFORTRAN_EXCEPTION_H
#define LFORTRAN_EXCEPTION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LFORTRAN_NO_EXCEPTION    = 0,
    LFORTRAN_RUNTIME_ERROR   = 1,
    LFORTRAN_EXCEPTION       = 2,
    LFORTRAN_PARSER_ERROR    = 4,
    LFORTRAN_ASSERT_FAILED   = 7,
    LFORTRAN_ASSEMBLER_ERROR = 8,
} lfortran_exceptions_t;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <exception>
#include <string>
#include <libasr/location.h>
#include <libasr/config.h>
#include <libasr/stacktrace.h>
#include <libasr/diagnostics.h>

namespace LCompilers {

/*
    This Error structure is returned in Result when failure happens.

    The diagnostic messages contain warnings and error(s). In the past when
    just one error was returned, it made sense to return it as part of the
    Error structure in Result. However now when warnings are also reported,
    those we want to return in any case. Also since multiple errors can be
    reported, they are now storted independently of the exception that is
    internally used to abort the analysis (typically a visitor pattern).

    For the above reasons the diagnostic messages are now returned as an
    argument, independently of the Result<T>, and they can contain messages on
    success also. On failure they contain at least one error message (and in
    addition can contain warnings and more error messages).

    Consequently, we do not currently store anything in the Error structure
    below.
*/
struct Error {
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

class LCompilersException : public std::exception
{
    std::string m_msg;
    lfortran_exceptions_t ec;
    std::vector<StacktraceItem> m_stacktrace_addresses;
public:
    LCompilersException(const std::string &msg, lfortran_exceptions_t error)
        : m_msg{msg}, ec{error}, m_stacktrace_addresses{get_stacktrace_addresses()}
    { }
    LCompilersException(const std::string &msg)
        : LCompilersException(msg, LFORTRAN_EXCEPTION)
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
                return "LCompilersException";
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

class AssertFailed : public LCompilersException
{
public:
    AssertFailed(const std::string &msg)
        : LCompilersException(msg, LFORTRAN_ASSERT_FAILED)
    {
    }
};

class AssemblerError : public LCompilersException
{
public:
    AssemblerError(const std::string &msg)
        : LCompilersException(msg, LFORTRAN_ASSEMBLER_ERROR)
    {
    }
};

template<typename T>
static inline T TRY(Result<T> result) {
    if (result.ok) {
        return result.result;
    } else {
        throw LCompilersException("Internal Compiler Error: TRY failed, error was not handled explicitly");
    }
}


} // namespace LCompilers

#endif // __cplusplus
#endif // LFORTRAN_EXCEPTION_H
