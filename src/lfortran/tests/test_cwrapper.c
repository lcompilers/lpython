#include <lfortran/cwrapper.h>

void test_parse()
{
    const char *s="4+5";
    LFortranCParser *l;
    l = lfortran_parser_new();
    lfortran_exceptions_t r;
    r = lfortran_parser_parse(l, s);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
    lfortran_parser_free(l);
}

int main()
{
    test_parse();
    return 0;
}
