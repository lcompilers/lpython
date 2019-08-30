cimport cwrapper

cdef class Parser(object)

cdef class AST(object):
    cdef cwrapper.lfortran_ast_t *thisptr
    cdef Parser parser

    @staticmethod
    cdef create(Parser p, cwrapper.lfortran_ast_t *ast):
        cdef AST a = AST.__new__(AST)
        a.parser = p
        a.thisptr = ast
        return a

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

cdef class Parser(object):
    cdef cwrapper.LFortranCParser *h;

    def __cinit__(self):
        self.h = cwrapper.lfortran_parser_new()

    def __dealloc__(self):
        cwrapper.lfortran_parser_free(self.h)

    def parse(self, s):
        sb = s.encode()
        cdef char *str = sb
        cdef cwrapper.lfortran_ast_t *ast;
        e = cwrapper.lfortran_parser_parse(self.h, str, &ast)
        if (e != cwrapper.LFORTRAN_NO_EXCEPTION):
            raise Exception("parser_parse failed")
        return AST.create(self, ast)

def parse(s):
    p = Parser()
    a = p.parse(s)
    s = a.pickle()
    return s
