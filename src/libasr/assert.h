#ifndef LFORTRAN_ASSERT_H
#define LFORTRAN_ASSERT_H

// LFORTRAN_ASSERT uses internal functions to perform as assert
// so that there is no effect with NDEBUG
#include <libasr/config.h>
#include <libasr/exception.h>
#if defined(WITH_LFORTRAN_ASSERT)

#include <iostream>

#if !defined(LFORTRAN_ASSERT)
#define stringize(s) #s
#define XSTR(s) stringize(s)
#if defined(HAVE_LFORTRAN_STACKTRACE)
#define LFORTRAN_ASSERT(cond)                                                  \
    {                                                                          \
        if (!(cond)) {                                                         \
            throw LFortran::AssertFailed(XSTR(cond));                  \
        }                                                                      \
    }
#else
#define LFORTRAN_ASSERT(cond)                                                  \
    {                                                                          \
        if (!(cond)) {                                                         \
            std::cerr << "LFORTRAN_ASSERT failed: " << __FILE__                \
                      << "\nfunction " << __func__ << "(), line number "       \
                      << __LINE__ << " at \n"                                  \
                      << XSTR(cond) << "\n";                                   \
            abort();                                                           \
        }                                                                      \
    }
#endif // defined(HAVE_LFORTRAN_STACKTRACE)
#endif // !defined(LFORTRAN_ASSERT)

#if !defined(LFORTRAN_ASSERT_MSG)
#define LFORTRAN_ASSERT_MSG(cond, msg)                                        \
    {                                                                          \
        if (!(cond)) {                                                         \
            std::cerr << "LFORTRAN_ASSERT failed: " << __FILE__               \
                      << "\nfunction " << __func__ << "(), line number "       \
                      << __LINE__ << " at \n"                                  \
                      << XSTR(cond) << "\n"                                    \
                      << "ERROR MESSAGE:\n"                                    \
                      << msg << "\n";                                          \
            abort();                                                           \
        }                                                                      \
    }
#endif // !defined(LFORTRAN_ASSERT_MSG)

#else // defined(WITH_LFORTRAN_ASSERT)

#define LFORTRAN_ASSERT(cond)
#define LFORTRAN_ASSERT_MSG(cond, msg)

#endif // defined(WITH_LFORTRAN_ASSERT)

#define LFORTRAN_ERROR(description)                                           \
    std::cerr << description;                                                  \
    std::cerr << "\n";                                                         \
    abort();

#endif // LFORTRAN_ASSERT_H
