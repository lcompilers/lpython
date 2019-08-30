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

def parse(s):
    a = AST()
    a.h = cwrapper.lfortran_parser_new()
    sb = s.encode()
    cdef char *str = sb
    cdef cwrapper.lfortran_ast_t *ast;
    e = cwrapper.lfortran_parser_parse(a.h, str, &ast)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("parser_parse failed")
    a.thisptr = ast
    s = a.pickle()
    cwrapper.lfortran_parser_free(a.h)
    return s
