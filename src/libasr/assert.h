#ifndef LCOMPILERS_ASSERT_H
#define LCOMPILERS_ASSERT_H

// LCOMPILERS_ASSERT uses internal functions to perform as assert
// so that there is no effect with NDEBUG
#include <libasr/config.h>
#include <libasr/exception.h>
#if defined(WITH_LCOMPILERS_ASSERT)

#include <iostream>

#if !defined(LCOMPILERS_ASSERT)
#define stringize(s) #s
#define XSTR(s) stringize(s)
#if defined(HAVE_LCOMPILERS_STACKTRACE)
#define LCOMPILERS_ASSERT(cond)                                                  \
    {                                                                          \
        if (!(cond)) {                                                         \
            throw LCompilers::AssertFailed(XSTR(cond));                  \
        }                                                                      \
    }
#else
#define LCOMPILERS_ASSERT(cond)                                                  \
    {                                                                          \
        if (!(cond)) {                                                         \
            std::cerr << "LCOMPILERS_ASSERT failed: " << __FILE__                \
                      << "\nfunction " << __func__ << "(), line number "       \
                      << __LINE__ << " at \n"                                  \
                      << XSTR(cond) << "\n";                                   \
            abort();                                                           \
        }                                                                      \
    }
#endif // defined(HAVE_LCOMPILERS_STACKTRACE)
#endif // !defined(LCOMPILERS_ASSERT)

#if !defined(LCOMPILERS_ASSERT_MSG)
#define LCOMPILERS_ASSERT_MSG(cond, msg)                                        \
    {                                                                          \
        if (!(cond)) {                                                         \
            std::cerr << "LCOMPILERS_ASSERT failed: " << __FILE__               \
                      << "\nfunction " << __func__ << "(), line number "       \
                      << __LINE__ << " at \n"                                  \
                      << XSTR(cond) << "\n"                                    \
                      << "ERROR MESSAGE:\n"                                    \
                      << msg << "\n";                                          \
            abort();                                                           \
        }                                                                      \
    }
#endif // !defined(LCOMPILERS_ASSERT_MSG)

#else // defined(WITH_LCOMPILERS_ASSERT)

#define LCOMPILERS_ASSERT(cond)
#define LCOMPILERS_ASSERT_MSG(cond, msg)

#endif // defined(WITH_LCOMPILERS_ASSERT)

#define LCOMPILERS_ERROR(description)                                           \
    std::cerr << description;                                                  \
    std::cerr << "\n";                                                         \
    abort();

#endif // LCOMPILERS_ASSERT_H
