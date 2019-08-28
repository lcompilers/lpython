cimport cwrapper

def parse(s):
    cdef cwrapper.LFortranCParser *h
    h = cwrapper.lfortran_parser_new()
    cdef cwrapper.lfortran_exceptions_t e;
    cdef const char *str = s;
    e = cwrapper.lfortran_parser_parse(h, str)
    cwrapper.lfortran_parser_free(h)
