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
            name = char (char | dig)*;
            integer = dig+;

            * {
                throw LFortran::TokenizerError("Unknown token: '"
                    + token() + "'");
            }
            end { return yytokentype::END_OF_FILE; }
            whitespace { continue; }

            // Keywords
            'abstract' { return yytokentype::KW_ABSTRACT; }
            'all' { return yytokentype::KW_ALL; }
            'allocatable' { return yytokentype::KW_ALLOCATABLE; }
            'allocate' { return yytokentype::KW_ALLOCATE; }
            'assignment' { return yytokentype::KW_ASSIGNMENT; }
            'associate' { return yytokentype::KW_ASSOCIATE; }
            'asynchronous' { return yytokentype::KW_ASYNCHRONOUS; }
            'backspace' { return yytokentype::KW_BACKSPACE; }
            'bind' { return yytokentype::KW_BIND; }
            'block' { return yytokentype::KW_BLOCK; }
            'call' { return yytokentype::KW_CALL; }
            'case' { return yytokentype::KW_CASE; }
            'character' { return yytokentype::KW_CHARACTER; }
            'class' { return yytokentype::KW_CLASS; }
            'close' { return yytokentype::KW_CLOSE; }
            'codimension' { return yytokentype::KW_CODIMENSION; }
            'common' { return yytokentype::KW_COMMON; }
            'complex' { return yytokentype::KW_COMPLEX; }
            'concurrent' { return yytokentype::KW_CONCURRENT; }
            'contains' { return yytokentype::KW_CONTAINS; }
            'contiguous' { return yytokentype::KW_CONTIGUOUS; }
            'continue' { return yytokentype::KW_CONTINUE; }
            'critical' { return yytokentype::KW_CRITICAL; }
            'cycle' { return yytokentype::KW_CYCLE; }
            'data' { return yytokentype::KW_DATA; }
            'deallocate' { return yytokentype::KW_DEALLOCATE; }
            'default' { return yytokentype::KW_DEFAULT; }
            'deferred' { return yytokentype::KW_DEFERRED; }
            'dimension' { return yytokentype::KW_DIMENSION; }
            'do' { return yytokentype::KW_DO; }
            'dowhile' { return yytokentype::KW_DOWHILE; }
            'double' { return yytokentype::KW_DOUBLE; }
            'elemental' { return yytokentype::KW_ELEMENTAL; }
            'else' { return yytokentype::KW_ELSE; }
            'end' { return yytokentype::KW_END; }
            'entry' { return yytokentype::KW_ENTRY; }
            'enum' { return yytokentype::KW_ENUM; }
            'enumerator' { return yytokentype::KW_ENUMERATOR; }
            'equivalence' { return yytokentype::KW_EQUIVALENCE; }
            'errmsg' { return yytokentype::KW_ERRMSG; }
            'error' { return yytokentype::KW_ERROR; }
            'exit' { return yytokentype::KW_EXIT; }
            'extends' { return yytokentype::KW_EXTENDS; }
            'external' { return yytokentype::KW_EXTERNAL; }
            'file' { return yytokentype::KW_FILE; }
            'final' { return yytokentype::KW_FINAL; }
            'flush' { return yytokentype::KW_FLUSH; }
            'forall' { return yytokentype::KW_FORALL; }
            'format' { return yytokentype::KW_FORMAT; }
            'formatted' { return yytokentype::KW_FORMATTED; }
            'function' { return yytokentype::KW_FUNCTION; }
            'generic' { return yytokentype::KW_GENERIC; }
            'go'  { return yytokentype::KW_GO; }
            'if' { return yytokentype::KW_IF; }
            'implicit' { return yytokentype::KW_IMPLICIT; }
            'import' { return yytokentype::KW_IMPORT; }
            'impure' { return yytokentype::KW_IMPURE; }
            'in' { return yytokentype::KW_IN; }
            'include' { return yytokentype::KW_INCLUDE; }
            'inout' { return yytokentype::KW_INOUT; }
            'inquire' { return yytokentype::KW_INQUIRE; }
            'integer' { return yytokentype::KW_INTEGER; }
            'intent' { return yytokentype::KW_INTENT; }
            'interface' { return yytokentype::KW_INTERFACE; }
            'intrinsic' { return yytokentype::KW_INTRINSIC; }
            'is' { return yytokentype::KW_IS; }
            'kind' { return yytokentype::KW_KIND; }
            'len' { return yytokentype::KW_LEN; }
            'local' { return yytokentype::KW_LOCAL; }
            'local_init' { return yytokentype::KW_LOCAL_INIT; }
            'logical' { return yytokentype::KW_LOGICAL; }
            'module' { return yytokentype::KW_MODULE; }
            'mold' { return yytokentype::KW_MOLD; }
            'name' { return yytokentype::KW_NAME; }
            'namelist' { return yytokentype::KW_NAMELIST; }
            'nopass' { return yytokentype::KW_NOPASS; }
            'non_intrinsic' { return yytokentype::KW_NON_INTRINSIC; }
            'non_overridable' { return yytokentype::KW_NON_OVERRIDABLE; }
            'non_recursive' { return yytokentype::KW_NON_RECURSIVE; }
            'none' { return yytokentype::KW_NONE; }
            'nullify' { return yytokentype::KW_NULLIFY; }
            'only' { return yytokentype::KW_ONLY; }
            'open' { return yytokentype::KW_OPEN; }
            'operator' { return yytokentype::KW_OPERATOR; }
            'optional' { return yytokentype::KW_OPTIONAL; }
            'out' { return yytokentype::KW_OUT; }
            'parameter' { return yytokentype::KW_PARAMETER; }
            'pass' { return yytokentype::KW_PASS; }
            'pointer' { return yytokentype::KW_POINTER; }
            'precision' { return yytokentype::KW_PRECISION; }
            'print' { return yytokentype::KW_PRINT; }
            'private' { return yytokentype::KW_PRIVATE; }
            'procedure' { return yytokentype::KW_PROCEDURE; }
            'program' { return yytokentype::KW_PROGRAM; }
            'protected' { return yytokentype::KW_PROTECTED; }
            'public' { return yytokentype::KW_PUBLIC; }
            'pure' { return yytokentype::KW_PURE; }
            'quiet' { return yytokentype::KW_QUIET; }
            'rank' { return yytokentype::KW_RANK; }
            'read' { return yytokentype::KW_READ; }
            'real' {return yytokentype::KW_REAL; }
            'recursive' { return yytokentype::KW_RECURSIVE; }
            'result' { return yytokentype::KW_RESULT; }
            'return' { return yytokentype::KW_RETURN; }
            'rewind' { return yytokentype::KW_REWIND; }
            'save' { return yytokentype::KW_SAVE; }
            'select' { return yytokentype::KW_SELECT; }
            'sequence' { return yytokentype::KW_SEQUENCE; }
            'shared' { return yytokentype::KW_SHARED; }
            'source' { return yytokentype::KW_SOURCE; }
            'stat' { return yytokentype::KW_STAT; }
            'stop' { return yytokentype::KW_STOP; }
            'submodule' { return yytokentype::KW_SUBMODULE; }
            'subroutine' { return yytokentype::KW_SUBROUTINE; }
            'target' { return yytokentype::KW_TARGET; }
            'team' { return yytokentype::KW_TEAM; }
            'team_number' { return yytokentype::KW_TEAM_NUMBER; }
            'then' { return yytokentype::KW_THEN; }
            'to' { return yytokentype::KW_TO; }
            'type' { return yytokentype::KW_TYPE; }
            'unformatted' { return yytokentype::KW_UNFORMATTED; }
            'use' { return yytokentype::KW_USE; }
            'value' { return yytokentype::KW_VALUE; }
            'volatile' { return yytokentype::KW_VOLATILE; }
            'where' { return yytokentype::KW_WHERE; }
            'while' { return yytokentype::KW_WHILE; }
            'write' { return yytokentype::KW_WRITE; }

            // Tokens
            newline { return yytokentype::TK_NEWLINE; }

            // Single character symbols
            symbols1 = "("|")"|"["|"]"|"+"|"-"|"="|":"|";"|"/"|"%"|","|"*"|"|";
            symbols1 { return tok[0]; }

            // Multiple character symbols
            ".." { return yytokentype::TK_DBL_DOT; }
            "::" { return yytokentype::TK_DBL_COLON; }
            "**" { return yytokentype::TK_POW; }
            "//" { return yytokentype::TK_CONCAT; }
            "=>" { return yytokentype::TK_ARROW; }

            // Relational operators
            ".eq." | "==" { return yytokentype::TK_EQ; }
            ".ne." | "/=" { return yytokentype::TK_NE; }
            ".lt." | "<"  { return yytokentype::TK_LT; }
            ".le." | "<=" { return yytokentype::TK_LE; }
            ".gt." | ">"  { return yytokentype::TK_GT; }
            ".ge." | ">=" { return yytokentype::TK_GE; }

            // Logical operators
            ".not."  { return yytokentype::TK_NOT; }
            ".and."  { return yytokentype::TK_AND; }
            ".or."   { return yytokentype::TK_OR; }
            ".eqv."  { return yytokentype::TK_EQV; }
            ".neqv." { return yytokentype::TK_NEQV; }


            integer {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    return yytokentype::TK_INTEGER;
                } else {
                    throw LFortran::TokenizerError("Integer too large");
                }
            }

            "."[a-z]+"." { yylval.string=token(); return yytokentype::TK_DEF_OP; }
            name { yylval.string=token(); return yytokentype::TK_NAME; }
        */
    }
}

} // namespace LFortran
