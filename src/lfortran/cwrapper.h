#ifndef CWRAPPER_H
#define CWRAPPER_H

#include <stdio.h>
#include <stdlib.h>

#include <lfortran/config.h>
#include <lfortran/exception.h>

#ifdef __cplusplus
extern "C" {
#endif

// Use LFORTRAN_C_ASSERT in C tests
#define LFORTRAN_C_ASSERT(cond)                                               \
    {                                                                          \
        if (0 == (cond)) {                                                     \
            printf("LFORTRAN_C_ASSERT failed: %s \nfunction %s (), line "     \
                   "number %d at\n%s\n",                                       \
                   __FILE__, __func__, __LINE__, #cond);                       \
            abort();                                                           \
        }                                                                      \
    }


typedef struct LFortranCParser LFortranCParser;

LFortranCParser *lfortran_parser_new();
void lfortran_parser_free(LFortranCParser *self);
lfortran_exceptions_t lfortran_parser_parse(LFortranCParser *self,
        const char *input);


#ifdef __cplusplus
}
#endif
#endif
