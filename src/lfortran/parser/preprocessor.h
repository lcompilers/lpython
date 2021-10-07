#ifndef LFORTRAN_SRC_PARSER_PREPROCESSOR_H
#define LFORTRAN_SRC_PARSER_PREPROCESSOR_H

#include <lfortran/exception.h>
#include <lfortran/parser/parser.h>

namespace LFortran
{

class CPreprocessor
{
public:
    std::string token(unsigned char *tok, unsigned char* cur) const;
    std::string run(const std::string &input, LocationManager &lm) const;

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
