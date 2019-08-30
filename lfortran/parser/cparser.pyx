cimport cwrapper

cdef class AST(object):
    cdef cwrapper.lfortran_ast_t *thisptr

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
    cdef cwrapper.LFortranCParser *h
    h = cwrapper.lfortran_parser_new()
    cdef cwrapper.lfortran_exceptions_t e
    sb = s.encode()
    cdef char *str = sb
    cdef cwrapper.lfortran_ast_t *ast;
    e = cwrapper.lfortran_parser_parse(h, str, &ast)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("parser_parse failed")
    cdef char *out
    e = cwrapper.lfortran_parser_pickle(ast, &out)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("parser_pickle failed")
    sout = out.decode()
    e = cwrapper.lfortran_str_free(out)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("str_free failed")
    cwrapper.lfortran_parser_free(h)
    return sout
