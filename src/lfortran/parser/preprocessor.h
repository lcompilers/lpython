#ifndef LFORTRAN_SRC_PARSER_PREPROCESSOR_H
#define LFORTRAN_SRC_PARSER_PREPROCESSOR_H

#include <libasr/exception.h>
#include <lfortran/utils.h>
#include <lfortran/parser/parser.h>

namespace LFortran
{

struct CPPMacro {
    /*
        Is the macro function-like.

        true:  #define f(a,b,c) a+b+c
        false: #define f something
    */
    bool function_like=false;
    std::vector<std::string> args; // Only used if function_like == true
    std::string expansion;
};

typedef std::map<std::string, CPPMacro> cpp_symtab;

class CPreprocessor
{
public:
    CompilerOptions &compiler_options;
    cpp_symtab macro_definitions;
    CPreprocessor(CompilerOptions &compiler_options);
    std::string token(unsigned char *tok, unsigned char* cur) const;
    std::string run(const std::string &input, LocationManager &lm,
        cpp_symtab &macro_definitions) const;
    std::string function_like_macro_expansion(
                std::vector<std::string> &def_args,
                std::string &expansion,
                std::vector<std::string> &call_args) const;

    // Return the current token's location
    void token_loc(Location &loc, unsigned char *tok, unsigned char* cur,
            unsigned char *string_start) const
    {
        loc.first = tok-string_start;
        loc.last = cur-string_start-1;
    }
};

} // namespace LFortran

#endif // LFORTRAN_SRC_PARSER_PREPROCESSOR_H
