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
    cur_line = cur;
    line_num = 1;
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

#define KW(x) token(yylval.string); RET(KW_##x);
#define RET(x) token_loc(loc); last_token=yytokentype::x; return yytokentype::x;

int Tokenizer::lex(YYSTYPE &yylval, Location &loc)
{
	unsigned long u;
    for (;;) {
        tok = cur;
        /*
        Re2c has an excellent documentation at:

        https://re2c.org/manual/manual_c.html

        The first paragraph there explains the basics:

        * If multiple rules match, the longest match takes precedence
        * If multiple rules match the same string, the earlier rule takes
          precedence
        * Default rule `*` should always be defined, it has the lowest priority
          regardless of its place and matches any code unit

        See the manual for more details.
        */
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
            string1 = (kind "_")? '"' ('""'|[^"\x00])* '"';
            string2 = (kind "_")? "'" ("''"|[^'\x00])* "'";
            comment = "!" [^\n\x00]*;
            ws_comment = whitespace? comment? "\n";

            * { token_loc(loc);
                std::string t = token();
                throw LFortran::TokenizerError("token '" + t
                    + "' is not recognized", loc, t);
            }
            end { RET(END_OF_FILE); }
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
            'elseif' { KW(ELSEIF) }
            'elsewhere' { KW(ELSEWHERE) }
            'end' { KW(END) }
            'end' whitespace 'forall' { KW(END_FORALL) }
            'endforall' { KW(ENDFORALL) }
            'end' whitespace 'if' { KW(END_IF) }
            'endif' { KW(ENDIF) }
            'end' whitespace 'interface' { KW(END_INTERFACE) }
            'endinterface' { KW(ENDINTERFACE) }
            'end' whitespace 'do' { KW(END_DO) }
            'enddo' { KW(ENDDO) }
            'end' whitespace 'where' { KW(END_WHERE) }
            'endwhere' { KW(ENDWHERE) }
            'entry' { KW(ENTRY) }
            'enum' { KW(ENUM) }
            'enumerator' { KW(ENUMERATOR) }
            'equivalence' { KW(EQUIVALENCE) }
            'errmsg' { KW(ERRMSG) }
            'error' { KW(ERROR) }
            'event' { KW(EVENT) }
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
            'in' whitespace 'out' { KW(IN_OUT) }
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
            'post' { KW(POST) }
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
            'reduce' { KW(REDUCE) }
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
            'sync' { KW(SYNC) }
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
            'wait' { KW(WAIT) }
            'where' { KW(WHERE) }
            'while' { KW(WHILE) }
            'write' { KW(WRITE) }

            // Tokens
            newline {
                    token_loc(loc); line_num++; cur_line=cur;
                    last_token = yytokentype::TK_NEWLINE;
                    return yytokentype::TK_NEWLINE;
            }

            // Single character symbols
            "(" { RET(TK_LPAREN) }
            "(" / "/=" { RET(TK_LPAREN) } // To parse "operator(/=)" correctly
            "(" / "/," { RET(TK_LPAREN) } // To parse "format(/,'xx')" correctly
            "(" / ("/" whitespace ",") { RET(TK_LPAREN) } // To parse "format(/ ,'xx')" correctly
            "(" / "//," { RET(TK_LPAREN) } // To parse "format(//,'xx')" correctly
            "(" / ("//" whitespace ",") { RET(TK_LPAREN) } // To parse "format(// ,'xx')" correctly
            "(" / "/)" { RET(TK_LPAREN) } // To parse "format(/)" correctly
            "(" / ("/" whitespace ")") { RET(TK_LPAREN) } // To parse "format(/ )" correctly
            ")" { RET(TK_RPAREN) }
            "[" | "(/" { RET(TK_LBRACKET) }
            "]" { RET(TK_RBRACKET) }
            "/)" { RET(TK_RBRACKET_OLD) }
            "+" { RET(TK_PLUS) }
            "-" { RET(TK_MINUS) }
            "=" { RET(TK_EQUAL) }
            ":" { RET(TK_COLON) }
            ";" { RET(TK_SEMICOLON) }
            "/" { RET(TK_SLASH) }
            "%" { RET(TK_PERCENT) }
            "," { RET(TK_COMMA) }
            "*" { RET(TK_STAR) }
            "|" { RET(TK_VBAR) }

            // Multiple character symbols
            ".." { RET(TK_DBL_DOT) }
            "::" { RET(TK_DBL_COLON) }
            "**" { RET(TK_POW) }
            "//" { RET(TK_CONCAT) }
            "=>" { RET(TK_ARROW) }

            // Relational operators
            '.eq.' | "==" { RET(TK_EQ) }
            '.ne.' | "/=" { RET(TK_NE) }
            '.lt.' | "<"  { RET(TK_LT) }
            '.le.' | "<=" { RET(TK_LE) }
            '.gt.' | ">"  { RET(TK_GT) }
            '.ge.' | ">=" { RET(TK_GE) }

            // Logical operators
            '.not.'  { RET(TK_NOT) }
            '.and.'  { RET(TK_AND) }
            '.or.'   { RET(TK_OR) }
            '.eqv.'  { RET(TK_EQV) }
            '.neqv.' { RET(TK_NEQV) }

            // True/False

            '.true.' ("_" kind)? { RET(TK_TRUE) }
            '.false.' ("_" kind)? { RET(TK_FALSE) }

            // This is needed to ensure that 2.op.3 gets tokenized as
            // TK_INTEGER(2), TK_DEFOP(.op.), TK_INTEGER(3), and not
            // TK_REAL(2.), TK_NAME(op), TK_REAL(.3). The `.op.` can be a
            // built-in or custom defined operator, such as: `.eq.`, `.not.`,
            // or `.custom.`.
            integer / defop {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    RET(TK_INTEGER)
                } else {
                    token_loc(loc);
                    std::string t = token();
                    throw LFortran::TokenizerError("Integer too large",
                        loc, t);
                }
            }


            real { token(yylval.string); RET(TK_REAL) }
            integer / (whitespace name) {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    if (last_token == yytokentype::TK_NEWLINE) {
                        RET(TK_LABEL)
                    } else {
                        RET(TK_INTEGER)
                    }
                } else {
                    token_loc(loc);
                    std::string t = token();
                    throw LFortran::TokenizerError("Integer too large",
                        loc, t);
                }
            }
            integer {
                if (lex_dec(tok, cur, u)) {
                    yylval.n = u;
                    RET(TK_INTEGER)
                } else {
                    token_loc(loc);
                    std::string t = token();
                    throw LFortran::TokenizerError("Integer too large",
                        loc, t);
                }
            }

            [bB] '"' [01]+ '"' { token(yylval.string); RET(TK_BOZ_CONSTANT) }
            [bB] "'" [01]+ "'" { token(yylval.string); RET(TK_BOZ_CONSTANT) }
            [oO] '"' [0-7]+ '"' { token(yylval.string); RET(TK_BOZ_CONSTANT) }
            [oO] "'" [0-7]+ "'" { token(yylval.string); RET(TK_BOZ_CONSTANT) }
            [zZ] '"' [0-9a-fA-F]+ '"' { token(yylval.string); RET(TK_BOZ_CONSTANT) }
            [zZ] "'" [0-9a-fA-F]+ "'" { token(yylval.string); RET(TK_BOZ_CONSTANT) }

            "&" ws_comment+ whitespace? "&"? {
                line_num++; cur_line=cur; continue;
            }

            comment / "\n" { token(yylval.string); RET(TK_COMMENT) }

            // Macros are ignored for now:
            "#" [^\n\x00]* "\n" { line_num++; cur_line=cur; continue; }

            // Include statements are ignored for now
            'include' whitespace string1 { continue; }
            'include' whitespace string2 { continue; }

            string1 { token_str(yylval.string); RET(TK_STRING) }
            string2 { token_str(yylval.string); RET(TK_STRING) }

            defop { token(yylval.string); RET(TK_DEF_OP) }
            name { token(yylval.string); RET(TK_NAME) }
        */
    }
}

} // namespace LFortran
