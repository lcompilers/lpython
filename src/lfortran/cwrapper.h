#ifndef CWRAPPER_H
#define CWRAPPER_H

#include <stdio.h>
#include <stdlib.h>

#include <libasr/config.h>
#include <libasr/exception.h>

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
typedef struct lfortran_ast_t lfortran_ast_t;

LFortranCParser *lfortran_parser_new();
void lfortran_parser_free(LFortranCParser *self);
lfortran_exceptions_t lfortran_parser_parse(LFortranCParser *self,
        const char *input, lfortran_ast_t **ast);
lfortran_exceptions_t lfortran_parser_pickle(lfortran_ast_t* ast,
        char **str);
lfortran_exceptions_t lfortran_str_free(char *str);


#ifdef __cplusplus
}
#endif
#endif
