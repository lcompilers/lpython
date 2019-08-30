cimport cwrapper

cdef class AST(object):
    cdef cwrapper.lfortran_ast_t *thisptr
    cdef cwrapper.LFortranCParser *h;

    def pickle(self):
        cdef char *out
        e = cwrapper.lfortran_parser_pickle(self.thisptr, &out)
        if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
            raise Exception("parser_pickle failed")
        sout = out.decode()
        e = cwrapper.lfortran_str_free(out)
        if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
            raise Exception("str_free failed")
        return sout

    def __cinit__(self):
        self.h = cwrapper.lfortran_parser_new()
#        self.thisptr = 0;

    def __dealloc__(self):
        cwrapper.lfortran_parser_free(self.h)

def parse(s):
    a = AST()
    sb = s.encode()
    cdef char *str = sb
    cdef cwrapper.lfortran_ast_t *ast;
    e = cwrapper.lfortran_parser_parse(a.h, str, &ast)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("parser_parse failed")
    a.thisptr = ast
    s = a.pickle()
    return s
