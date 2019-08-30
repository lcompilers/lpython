cdef extern from "lfortran/cwrapper.h":

    ctypedef enum lfortran_exceptions_t:
        LFORTRAN_NO_EXCEPTION
        LFORTRAN_RUNTIME_ERROR
        LFORTRAN_TOKENIZER_ERROR
        LFORTRAN_PARSER_ERROR

    ctypedef struct LFortranCParser:
        pass
    ctypedef struct lfortran_ast_t:
        pass

    LFortranCParser *lfortran_parser_new() nogil
    void lfortran_parser_free(LFortranCParser *self) nogil
    lfortran_exceptions_t lfortran_parser_parse(LFortranCParser *self,
            const char *input, lfortran_ast_t **ast) nogil
    lfortran_exceptions_t lfortran_parser_pickle(lfortran_ast_t* ast,
            char **str) nogil
    lfortran_exceptions_t lfortran_str_free(char *str) nogil
