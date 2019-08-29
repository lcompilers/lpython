#include <string.h>

#include <lfortran/cwrapper.h>

void test_parse()
{
    const char *s="4+5";
    LFortranCParser *l;
    l = lfortran_parser_new();
    lfortran_exceptions_t r;
    char *out;
    r = lfortran_parser_parse(l, s, &out);
    lfortran_parser_free(l);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
    LFORTRAN_C_ASSERT(strcmp(out, "(+ 4 5)") == 0);
    r = lfortran_str_free(out);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
}

int main()
{
    test_parse();
    return 0;
}
