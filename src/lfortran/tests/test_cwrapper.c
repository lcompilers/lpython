#include <string.h>

#include <lfortran/cwrapper.h>

void test_parse()
{
    const char *s="4+5";
    LFortranCParser *l;
    l = lfortran_parser_new();
    lfortran_exceptions_t r;
    lfortran_ast_t *ast;
    r = lfortran_parser_parse(l, s, &ast);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
    char *out;
    r = lfortran_parser_pickle(ast, &out);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
    lfortran_parser_free(l);
    LFORTRAN_C_ASSERT(strcmp(out, "(+ 4 5)") == 0);
    r = lfortran_str_free(out);
    LFORTRAN_C_ASSERT(r == LFORTRAN_NO_EXCEPTION);
}

int main()
{
    test_parse();
    return 0;
}
