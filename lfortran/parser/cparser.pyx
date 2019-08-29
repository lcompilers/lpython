cimport cwrapper

def parse(s):
    cdef cwrapper.LFortranCParser *h
    h = cwrapper.lfortran_parser_new()
    cdef cwrapper.lfortran_exceptions_t e
    sb = s.encode()
    cdef char *str = sb
    cdef char *out
    e = cwrapper.lfortran_parser_parse(h, str, &out)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("parser_parse failed")
    sout = out.decode()
    e = cwrapper.lfortran_str_free(out)
    if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
        raise Exception("str_free failed")
    cwrapper.lfortran_parser_free(h)
    return sout
