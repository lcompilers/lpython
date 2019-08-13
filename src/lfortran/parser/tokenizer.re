#include "tokenizer.h"
#include "parser.tab.hh"

namespace LFortran
{

void Tokenizer::set_string(const std::string &str)
{
    // The input string must be NULL terminated, otherwise the tokenizer will
    // not detect the end of string. After C++11, the std::string is guaranteed
    // to end with \0, but we check this here just in case.
    LFORTRAN_ASSERT(str[str.size()] == '\0');
    cur = (unsigned char *)(&str[0]);
}

int Tokenizer::lex(YYSTYPE &yylval)
{
    for (;;) {
        tok = cur;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            whitespace = [ \t\v\n\r]+;
            dig = [0-9];
            char =  [\x80-\xff] | [a-zA-Z_];
            operators = "-"|"+"|"/"|"("|")"|"*"|",";

            pows = "**"|"@";
            ident = char (char | dig)*;
            numeric = dig+;

            * {
                throw LFortran::TokenizerError("Unknown token: '"
                    + token() + "'");
            }
            end { return yytokentype::END_OF_FILE; }
            whitespace { continue; }

            // FIXME:
            operators { return tok[0]; }
            pows { return yytokentype::POW; }
            ident { yylval.string=token(); return yytokentype::IDENTIFIER; }
            numeric { yylval.string=token(); return yytokentype::NUMERIC; }
        */
    }
}

} // namespace LFortran
