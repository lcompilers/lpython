#include <limits>

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

template<int base>
bool adddgt(unsigned long &u, unsigned long d)
{
    if (u > (std::numeric_limits<unsigned long>::max() - d) / base) {
        return false;
    }
    u = u * base + d;
    return true;
}

bool lex_dec(const unsigned char *s, const unsigned char *e, unsigned long &u)
{
    for (u = 0; s < e; ++s) {
        if (!adddgt<10>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

int Tokenizer::lex(YYSTYPE &yylval)
{
	unsigned long u;
    for (;;) {
        tok = cur;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            whitespace = [ \t\v\r]+;
            newline = "\n";
            dig = [0-9];
            char =  [a-zA-Z_];
            operators = "-"|"+"|"/"|"("|")"|"*"|","|"="|";";

            pows = "**"|"@";
            ident = char (char | dig)*;
            numeric = dig+;

            * {
                throw LFortran::TokenizerError("Unknown token: '"
                    + token() + "'");
            }
            end { return yytokentype::END_OF_FILE; }
            whitespace { continue; }
            newline { return yytokentype::KW_NEWLINE; }

            // keywords
            'exit'       { return yytokentype::KW_EXIT; }
            'subroutine' { return yytokentype::KW_SUBROUTINE; }

            operators { return tok[0]; }
            pows { return yytokentype::POW; }
            ident { yylval.string=token(); return yytokentype::IDENTIFIER; }
            numeric {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    return yytokentype::NUMERIC;
                } else {
                    throw LFortran::TokenizerError("Integer too large");
                }
            }
        */
    }
}

} // namespace LFortran
