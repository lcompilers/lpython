#include <cstdlib>
#include <cstring>

#include <lfortran/cwrapper.h>
#include <lfortran/ast.h>
#include <lfortran/parser/alloc.h>
#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>


extern "C" {

#define CWRAPPER_BEGIN try {

#define CWRAPPER_END                                                           \
    return LFORTRAN_NO_EXCEPTION;                                              \
    }                                                                          \
    catch (LFortran::LFortranException & e)                                    \
    {                                                                          \
        return e.error_code();                                                 \
    }                                                                          \
    catch (...)                                                                \
    {                                                                          \
        return LFORTRAN_RUNTIME_ERROR;                                         \
    }




struct LFortranCParser {
    Allocator al;
    LFortranCParser() : al{1024*1024} {}
};

LFortranCParser *lfortran_parser_new()
{
    return new LFortranCParser;
}

void lfortran_parser_free(LFortranCParser *self)
{
    delete self;
}

lfortran_exceptions_t lfortran_parser_parse(LFortranCParser *self,
        const char *input)
{
    CWRAPPER_BEGIN

    LFortran::AST::ast_t* result;
    result = LFortran::parse(self->al, input);
    std::string p = LFortran::pickle(*result);
    std::cout << p << std::endl;

    CWRAPPER_END
}


} // extern "C"
