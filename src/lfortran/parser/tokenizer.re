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

#define KW(x) yylval.string=token(); return yytokentype::KW_##x;

int Tokenizer::lex(YYSTYPE &yylval, Location &loc)
{
	unsigned long u;
    for (;;) {
        tok = cur;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:define:YYCTXMARKER = ctxmar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            whitespace = [ \t\v\r]+;
            newline = "\n";
            digit = [0-9];
            char =  [a-zA-Z_];
            name = char (char | digit)*;
            defop = "."[a-z]+".";
            kind = digit+ | name;
            significand = (digit+"."digit*) | ("."digit+);
            exp = [edED][-+]? digit+;
            integer = digit+ ("_" kind)?;
            real = ((significand exp?) | (digit+ exp)) ("_" kind)?;

            * {
                throw LFortran::TokenizerError("Unknown token: '"
                    + token() + "'");
            }
            end { return yytokentype::END_OF_FILE; }
            whitespace { continue; }

            // Keywords
            'abstract' { KW(ABSTRACT) }
            'all' { KW(ALL) }
            'allocatable' { KW(ALLOCATABLE) }
            'allocate' { KW(ALLOCATE) }
            'assignment' { KW(ASSIGNMENT) }
            'associate' { KW(ASSOCIATE) }
            'asynchronous' { KW(ASYNCHRONOUS) }
            'backspace' { KW(BACKSPACE) }
            'bind' { KW(BIND) }
            'block' { KW(BLOCK) }
            'call' { KW(CALL) }
            'case' { KW(CASE) }
            'character' { KW(CHARACTER) }
            'class' { KW(CLASS) }
            'close' { KW(CLOSE) }
            'codimension' { KW(CODIMENSION) }
            'common' { KW(COMMON) }
            'complex' { KW(COMPLEX) }
            'concurrent' { KW(CONCURRENT) }
            'contains' { KW(CONTAINS) }
            'contiguous' { KW(CONTIGUOUS) }
            'continue' { KW(CONTINUE) }
            'critical' { KW(CRITICAL) }
            'cycle' { KW(CYCLE) }
            'data' { KW(DATA) }
            'deallocate' { KW(DEALLOCATE) }
            'default' { KW(DEFAULT) }
            'deferred' { KW(DEFERRED) }
            'dimension' { KW(DIMENSION) }
            'do' { KW(DO) }
            'dowhile' { KW(DOWHILE) }
            'double' { KW(DOUBLE) }
            'elemental' { KW(ELEMENTAL) }
            'else' { KW(ELSE) }
            'end' { KW(END) }
            'entry' { KW(ENTRY) }
            'enum' { KW(ENUM) }
            'enumerator' { KW(ENUMERATOR) }
            'equivalence' { KW(EQUIVALENCE) }
            'errmsg' { KW(ERRMSG) }
            'error' { KW(ERROR) }
            'exit' { KW(EXIT) }
            'extends' { KW(EXTENDS) }
            'external' { KW(EXTERNAL) }
            'file' { KW(FILE) }
            'final' { KW(FINAL) }
            'flush' { KW(FLUSH) }
            'forall' { KW(FORALL) }
            'format' { KW(FORMAT) }
            'formatted' { KW(FORMATTED) }
            'function' { KW(FUNCTION) }
            'generic' { KW(GENERIC) }
            'go'  { KW(GO) }
            'if' { KW(IF) }
            'implicit' { KW(IMPLICIT) }
            'import' { KW(IMPORT) }
            'impure' { KW(IMPURE) }
            'in' { KW(IN) }
            'include' { KW(INCLUDE) }
            'inout' { KW(INOUT) }
            'inquire' { KW(INQUIRE) }
            'integer' { KW(INTEGER) }
            'intent' { KW(INTENT) }
            'interface' { KW(INTERFACE) }
            'intrinsic' { KW(INTRINSIC) }
            'is' { KW(IS) }
            'kind' { KW(KIND) }
            'len' { KW(LEN) }
            'local' { KW(LOCAL) }
            'local_init' { KW(LOCAL_INIT) }
            'logical' { KW(LOGICAL) }
            'module' { KW(MODULE) }
            'mold' { KW(MOLD) }
            'name' { KW(NAME) }
            'namelist' { KW(NAMELIST) }
            'nopass' { KW(NOPASS) }
            'non_intrinsic' { KW(NON_INTRINSIC) }
            'non_overridable' { KW(NON_OVERRIDABLE) }
            'non_recursive' { KW(NON_RECURSIVE) }
            'none' { KW(NONE) }
            'nullify' { KW(NULLIFY) }
            'only' { KW(ONLY) }
            'open' { KW(OPEN) }
            'operator' { KW(OPERATOR) }
            'optional' { KW(OPTIONAL) }
            'out' { KW(OUT) }
            'parameter' { KW(PARAMETER) }
            'pass' { KW(PASS) }
            'pointer' { KW(POINTER) }
            'precision' { KW(PRECISION) }
            'print' { KW(PRINT) }
            'private' { KW(PRIVATE) }
            'procedure' { KW(PROCEDURE) }
            'program' { KW(PROGRAM) }
            'protected' { KW(PROTECTED) }
            'public' { KW(PUBLIC) }
            'pure' { KW(PURE) }
            'quiet' { KW(QUIET) }
            'rank' { KW(RANK) }
            'read' { KW(READ) }
            'real' {KW(REAL) }
            'recursive' { KW(RECURSIVE) }
            'result' { KW(RESULT) }
            'return' { KW(RETURN) }
            'rewind' { KW(REWIND) }
            'save' { KW(SAVE) }
            'select' { KW(SELECT) }
            'sequence' { KW(SEQUENCE) }
            'shared' { KW(SHARED) }
            'source' { KW(SOURCE) }
            'stat' { KW(STAT) }
            'stop' { KW(STOP) }
            'submodule' { KW(SUBMODULE) }
            'subroutine' { KW(SUBROUTINE) }
            'target' { KW(TARGET) }
            'team' { KW(TEAM) }
            'team_number' { KW(TEAM_NUMBER) }
            'then' { KW(THEN) }
            'to' { KW(TO) }
            'type' { KW(TYPE) }
            'unformatted' { KW(UNFORMATTED) }
            'use' { KW(USE) }
            'value' { KW(VALUE) }
            'volatile' { KW(VOLATILE) }
            'where' { KW(WHERE) }
            'while' { KW(WHILE) }
            'write' { KW(WRITE) }

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

            // True/False

            ".true." ("_" kind)? { return yytokentype::TK_TRUE; }
            ".false." ("_" kind)? { return yytokentype::TK_FALSE; }

            // This is needed to ensure that 2.op.3 gets tokenized as
            // TK_INTEGER(2), TK_DEFOP(.op.), TK_INTEGER(3), and not
            // TK_REAL(2.), TK_NAME(op), TK_REAL(.3). The `.op.` can be a
            // built-in or custom defined operator, such as: `.eq.`, `.not.`,
            // or `.custom.`.
            integer / defop {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    return yytokentype::TK_INTEGER;
                } else {
                    throw LFortran::TokenizerError("Integer too large");
                }
            }


            real { return yytokentype::TK_REAL; }
            integer {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    return yytokentype::TK_INTEGER;
                } else {
                    throw LFortran::TokenizerError("Integer too large");
                }
            }

            (kind "_")? '"' ('""'|[^"\x00])* '"' { return yytokentype::TK_STRING; }
            (kind "_")? "'" ("''"|[^'\x00])* "'" { return yytokentype::TK_STRING; }

            defop { yylval.string=token(); return yytokentype::TK_DEF_OP; }
            name { yylval.string=token(); return yytokentype::TK_NAME; }
        */
    }
}

} // namespace LFortran
