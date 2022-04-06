#include <limits>
#include <iostream>
#include <lpython/parser/parser_exception.h>
#include <lpython/parser/tokenizer.h>
#include <lpython/parser/parser.tab.hh>
#include <libasr/bigint.h>

namespace LFortran
{

void lex_format(unsigned char *&cur, Location &loc,
        unsigned char *&start);

void Tokenizer::set_string(const std::string &str)
{
    // The input string must be NULL terminated, otherwise the tokenizer will
    // not detect the end of string. After C++11, the std::string is guaranteed
    // to end with \0, but we check this here just in case.
    LFORTRAN_ASSERT(str[str.size()] == '\0');
    cur = (unsigned char *)(&str[0]);
    string_start = cur;
    cur_line = cur;
    line_num = 1;
}

template<int base>
bool adddgt(uint64_t &u, uint64_t d)
{
    if (u > (std::numeric_limits<uint64_t>::max() - d) / base) {
        return false;
    }
    u = u * base + d;
    return true;
}

bool lex_dec(const unsigned char *s, const unsigned char *e, uint64_t &u)
{
    for (u = 0; s < e; ++s) {
        if (!adddgt<10>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

// Tokenizes integer of the kind 1234_ikind into `u` and `suffix`
// s ... the start of the integer
// e ... the character after the end
bool lex_int(const unsigned char *s, const unsigned char *e, uint64_t &u,
    Str &suffix)
{
    for (u = 0; s < e; ++s) {
        if (*s == '_') {
            s++;
            suffix.p = (char*) s;
            suffix.n = e-s;
            return true;
        } else if (!adddgt<10>(u, *s - 0x30u)) {
            return false;
        }
    }
    suffix.p = nullptr;
    suffix.n = 0;
    return true;
}

void lex_int_large(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u, Str &suffix)
{
    uint64_t ui;
    if (lex_int(s, e, ui, suffix)) {
        if (ui <= BigInt::MAX_SMALL_INT) {
            u.from_smallint(ui);
            return;
        }
    }
    const unsigned char *start = s;
    for (; s < e; ++s) {
        if (*s == '_') {
            s++;
            suffix.p = (char*) s;
            suffix.n = e-s;

            Str num;
            num.p = (char*)start;
            num.n = s-start-1;
            u.from_largeint(al, num);
            return;
        }
    }
    suffix.p = nullptr;
    suffix.n = 0;
    Str num;
    num.p = (char*)start;
    num.n = e-start;
    u.from_largeint(al, num);
}

uint64_t parse_int(const unsigned char *s)
{
    while (*s == ' ') s++;
    uint64_t u;
    for (u = 0; ; ++s) {
        if (*s >= '0' && *s <= '9') {
            if (!adddgt<10>(u, *s - 0x30u)) {
                return false;
            }
        } else {
            return u;
        }
    }
}

#define KW(x) token(yylval.string); RET(KW_##x);
#define RET(x) token_loc(loc); last_token=yytokentype::x; return yytokentype::x;
#define WARN_REL(x) add_rel_warning(diagnostics, yytokentype::TK_##x);

void Tokenizer::add_rel_warning(diag::Diagnostics &diagnostics, int rel_token) const {
    static const std::map<int, std::pair<std::string, std::string>> m = {
        {yytokentype::TK_EQ, {"==", ".eq."}},
        {yytokentype::TK_NE, {"/=", ".ne."}},
        {yytokentype::TK_LT, {"<",  ".lt."}},
        {yytokentype::TK_GT, {">",  ".gt."}},
        {yytokentype::TK_LE, {"<=", ".le."}},
        {yytokentype::TK_GE, {">=", ".ge."}},
    };
    const std::string rel_new = m.at(rel_token).first;
    const std::string rel_old = m.at(rel_token).second;
    Location loc;
    token_loc(loc);
    diagnostics.tokenizer_style_label(
        "Use '" + rel_new + "' instead of '" + rel_old + "'",
        {loc},
        "help: write this as '" + rel_new + "'");
}

int Tokenizer::lex(Allocator &al, YYSTYPE &yylval, Location &loc, diag::Diagnostics &diagnostics)
{
    if(dedent == 1) {
        // Removes the indent completely i.e., to level 0
        if(!indent_length.empty()) {
            indent_length.pop_back();
            return yytokentype::TK_DEDENT;
        } else {
            dedent = 0;
        }
    } else if(dedent == 2) {
        // Reduce the indent to `last_indent_length`
        long int sum = 0;
        for (auto& n : indent_length) sum += n;
        if(sum != last_indent_length) {
            indent_length.pop_back();
            loc.first = loc.last;
            return yytokentype::TK_DEDENT;
        } else {
            dedent = 0;
        }
    }


    for (;;) {
        tok = cur;

        /*
        Re2c has excellent documentation at:

        https://re2c.org/manual/manual_c.html

        The first paragraph there explains the basics:

        * If multiple rules match, the longest match takes precedence
        * If multiple rules match the same string, the earlier rule takes
          precedence
        * Default rule `*` should always be defined, it has the lowest priority
          regardless of its place and matches any code unit
        * We use the "Sentinel character" method for end of input:
            * The end of the input text is denoted with a null character \x00
            * Thus the null character cannot be part of the input otherwise
            * There is one rule to match \x00 to end the parser
            * No other rule is allowed to match \x00, otherwise the re2c block
              would parse past the end of the string and segfaults
            * A special case of the previous point are negated character
              ranges, such as [^"\x00], where one must include \x00 in it to
              ensure this rule does not match \x00 (all other rules simply do
              not mention \x00)
            * See the "Handling the end of input" section in the re2c
              documentation for more info

        The re2c block interacts with the rest of the code via just one pointer
        variable `cur`. On entering the re2c block, the `cur` variable must
        point to the first character of the token to be tokenized by the block.
        The re2c block below then executes on its own until a rule is matched:
        the action in {} is then executed. In that action `cur` points to the
        first character of the next token.

        Before the re2c block we save the current `cur` into `tok`, so that we
        can use `tok` and `cur` in the action in {} to extract the token that
        corresponds to the rule that got matched:

        * `tok` points to the first character of the token
        * `cur-1` points to the last character of the token
        * `cur` points to the first character of the next token
        * `cur-tok` is the length of the token

        In the action, we do one of:

        * call `continue` which executes another cycle in the for loop (which
          will parse the next token); we use this to skip a token
        * call `return` which returns from this function; we return a token
        * throw an exception (terminates the tokenizer)

        In the first two cases, `cur` points to first character of the next
        token, which becomes `tok` at the next iteration of the loop (either
        right away after `continue` or after the `lex` function is called again
        after `return`).

        See the manual for more details.
        */


        // These two variables are needed by the re2c block below internally,
        // initialization is not needed. One can think of them as local
        // variables of the re2c block.
        unsigned char *mar, *ctxmar;
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
            defop = "."[a-zA-Z]+".";
            kind = digit+ | name;
            significand = (digit+"."digit*) | ("."digit+);
            exp = [edED][-+]? digit+;
            integer = digit+ ("_" kind)?;
            real = ((significand exp?) | (digit+ exp)) ("_" kind)?;
            string1 = '"' ('""'|[^"\x00])* '"';
            string2 = "'" ("''"|[^'\x00])* "'";
            comment = "#" [^\n\x00]*;
            docstring = newline whitespace? string1 | string2;
            ws_comment = whitespace? comment? newline;

            * { token_loc(loc);
                std::string t = token();
                throw parser_local::TokenizerError(diag::Diagnostic(
                    "Token '" + t + "' is not recognized",
                    diag::Level::Error, diag::Stage::Tokenizer, {
                        diag::Label("token not recognized", {loc})
                    })
                );
            }
            end { RET(END_OF_FILE); }
            whitespace {
                if (indent) {
                    indent = false;
                    indent_length.push_back(cur-tok);
                    last_indent_length = cur-tok;
                    RET(TK_INDENT);
                } else {
                    if(last_token == yytokentype::TK_NEWLINE && cur[0] != ' '
                            && last_indent_length > cur-tok) {
                        last_indent_length = cur-tok;
                        dedent = 2;
                        if (!indent_length.empty()) {
                            indent_length.pop_back();
                        }
                        RET(TK_DEDENT);
                    }
                }
                continue;
             }

            // Keywords
            'abstract' { KW(ABSTRACT) }
            'all' { KW(ALL) }
            'allocatable' { KW(ALLOCATABLE) }
            'allocate' { KW(ALLOCATE) }
            'assign' { KW(ASSIGN) }
            'assignment' { KW(ASSIGNMENT) }
            'associate' { KW(ASSOCIATE) }
            'asynchronous' { KW(ASYNCHRONOUS) }
            'backspace' { KW(BACKSPACE) }
            'bind' { KW(BIND) }
            'block' { KW(BLOCK) }
            'call' { KW(CALL) }
            'case' { KW(CASE) }
            'change' { KW(CHANGE) }
            'changeteam' { KW(CHANGE_TEAM) }
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
            'def' { KW(DEF) }
            'default' { KW(DEFAULT) }
            'deferred' { KW(DEFERRED) }
            'dimension' { KW(DIMENSION) }
            'do' / (whitespace digit+) {
                // This is a label do statement, we have to match the
                // corresponding continue base "end do".
                uint64_t n = parse_int(cur);
                enddo_label_stack.push_back(n);
                KW(DO);
            }
            'do' { KW(DO) }
            'dowhile' { KW(DOWHILE) }
            'double' { KW(DOUBLE) }
            'doubleprecision' { KW(DOUBLE_PRECISION) }
            'elemental' { KW(ELEMENTAL) }
            'else' { KW(ELSE) }
            'elseif' { KW(ELSEIF) }
            'elsewhere' { KW(ELSEWHERE) }

            'end' { KW(END) }

            'end' whitespace 'program' { KW(END_PROGRAM) }
            'endprogram' { KW(ENDPROGRAM) }

            'end' whitespace 'module' { KW(END_MODULE) }
            'endmodule' { KW(ENDMODULE) }

            'end' whitespace 'submodule' { KW(END_SUBMODULE) }
            'endsubmodule' { KW(ENDSUBMODULE) }

            'end' whitespace 'block' { KW(END_BLOCK) }
            'endblock' { KW(ENDBLOCK) }

            'end' whitespace 'block' whitespace 'data' { KW(END_BLOCK_DATA) }
            'endblock' whitespace 'data' { KW(END_BLOCK_DATA) }
            'end' whitespace 'blockdata' { KW(END_BLOCK_DATA) }
            'endblockdata' { KW(ENDBLOCKDATA) }

            'end' whitespace 'subroutine' { KW(END_SUBROUTINE) }
            'endsubroutine' { KW(ENDSUBROUTINE) }

            'end' whitespace 'function' { KW(END_FUNCTION) }
            'endfunction' { KW(ENDFUNCTION) }

            'end' whitespace 'procedure' { KW(END_PROCEDURE) }
            'endprocedure' { KW(ENDPROCEDURE) }

            'end' whitespace 'enum' { KW(END_ENUM) }
            'endenum' { KW(ENDENUM) }

            'end' whitespace 'select' { KW(END_SELECT) }
            'endselect' { KW(ENDSELECT) }

            'end' whitespace 'associate' { KW(END_ASSOCIATE) }
            'endassociate' { KW(ENDASSOCIATE) }

            'end' whitespace 'critical' { KW(END_CRITICAL) }
            'endcritical' { KW(ENDCRITICAL) }

            'end' whitespace 'team' { KW(END_TEAM) }
            'endteam' { KW(ENDTEAM) }

            'end' whitespace 'forall' { KW(END_FORALL) }
            'endforall' { KW(ENDFORALL) }

            'end' whitespace 'if' { KW(END_IF) }
            'endif' { KW(ENDIF) }

            'end' whitespace 'interface' { KW(END_INTERFACE) }
            'endinterface' { KW(ENDINTERFACE) }

            'end' whitespace 'type' { KW(END_TYPE) }
            'endtype' { KW(ENDTYPE) }

            'end' whitespace 'do' {
                if (enddo_newline_process) {
                    KW(CONTINUE)
                } else {
                    KW(END_DO)
                }
            }
            'enddo' {
                if (enddo_newline_process) {
                    KW(CONTINUE)
                } else {
                    KW(ENDDO)
                }
            }

            'end' whitespace 'where' { KW(END_WHERE) }
            'endwhere' { KW(ENDWHERE) }

            'end file' { KW(END_FILE) }
            'endfile' { KW(ENDFILE) }

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
            'format' {
                if (last_token == yytokentype::TK_LABEL) {
                    unsigned char *start;
                    lex_format(cur, loc, start);
                    yylval.string.p = (char*) start;
                    yylval.string.n = cur-start-1;
                    RET(TK_FORMAT)
                } else {
                    token(yylval.string);
                    RET(TK_NAME)
                }
            }
            'formatted' { KW(FORMATTED) }
            'form' { KW(FORM) }
            'formteam' { KW(FORM_TEAM) }
            'from' { KW(FROM) }
            'function' { KW(FUNCTION) }
            'generic' { KW(GENERIC) }
            'go' { KW(GO) }
            'goto' { KW(GOTO) }
            'if' { KW(IF) }
            'images' { KW(IMAGES) }
            'implicit' { KW(IMPLICIT) }
            'import' { KW(IMPORT) }
            'impure' { KW(IMPURE) }
            'in' { KW(IN) }
            'include' { KW(INCLUDE) }
            'inout' { KW(INOUT) }
            // 'in' whitespace 'out' { KW(IN_OUT) }
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
            'memory' { KW(MEMORY) }
            'module' { KW(MODULE) }
            'mold' { KW(MOLD) }
            'name' { KW(NAME) }
            'namelist' { KW(NAMELIST) }
            'new_index' { KW(NEW_INDEX) }
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
            'selectcase' { KW(SELECT_CASE) }
            'selectrank' { KW(SELECT_RANK) }
            'selecttype' { KW(SELECT_TYPE) }
            'sequence' { KW(SEQUENCE) }
            'shared' { KW(SHARED) }
            'source' { KW(SOURCE) }
            'stat' { KW(STAT) }
            'stop' { KW(STOP) }
            'submodule' { KW(SUBMODULE) }
            'subroutine' { KW(SUBROUTINE) }
            'sync' { KW(SYNC) }
            'syncall' { KW(SYNC_ALL) }
            'syncimages' { KW(SYNC_IMAGES) }
            'syncmemory' { KW(SYNC_MEMORY) }
            'syncteam' { KW(SYNC_TEAM) }
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
                if (last_token == yytokentype::TK_COLON) {
                    indent = true;
                } else if (cur[0] != ' ' && cur[0] != '\n'
                        && last_indent_length > cur-tok) {
                    last_indent_length = 0;
                    dedent = 1;
                }
                RET(TK_NEWLINE);
            }

            "\\" newline { continue; }

            // Single character symbols
            "(" { RET(TK_LPAREN) }
            // "(" / "/=" { RET(TK_LPAREN) } // To parse "operator(/=)" correctly
            "(" / "/)" { RET(TK_LPAREN) } // To parse "operator(/)" correctly
            // To parse "operator(/ )" correctly
            "(" / "/" whitespace ")" { RET(TK_LPAREN) }
            // To parse "operator(// )" correctly
            "(" / "//" whitespace ")" { RET(TK_LPAREN) }
            "(" / "//)" { RET(TK_LPAREN) } // To parse "operator(//)" correctly
            ")" { RET(TK_RPAREN) }
            "[" | "(/" { RET(TK_LBRACKET) }
            "]" { RET(TK_RBRACKET) }
            "/)" { RET(TK_RBRACKET_OLD) }
            "{" { RET(TK_LBRACE) }
            "}" { RET(TK_RBRACE) }
            "+" { RET(TK_PLUS) }
            "-" { RET(TK_MINUS) }
            "=" { RET(TK_EQUAL) }
            ":" { RET(TK_COLON) }
            ";" { RET(TK_SEMICOLON) }
            "/" { RET(TK_SLASH) }
            // "\\" { RET(TK_BACKSLASH) }
            "%" { RET(TK_PERCENT) }
            "," { RET(TK_COMMA) }
            "*" { RET(TK_STAR) }
            "|" { RET(TK_VBAR) }
            "&" { RET(TK_AMPERSAND) }
            "." { RET(TK_DOT) }
            "`" { RET(TK_BACKQUOTE) }
            "~" { RET(TK_TILDE) }
            "^" { RET(TK_CARET) }
            "@" { RET(TK_AT) }

            // Multiple character symbols
            ".." { RET(TK_DBL_DOT) }
            "::" { RET(TK_DBL_COLON) }
            "=>" { RET(TK_ARROW) }
            ">>" { RET(TK_RIGHTSHIFT) }
            "<<" { RET(TK_LEFTSHIFT) }
            "**" { RET(TK_POW) }
            "//" { RET(TK_FLOOR_DIV) }
            "+=" { RET(TK_PLUS_EQUAL) }
            "-=" { RET(TK_MIN_EQUAL) }
            "*=" { RET(TK_STAR_EQUAL) }
            "/=" { RET(TK_SLASH_EQUAL) }
            "%=" { RET(TK_PERCENT_EQUAL) }
            "&=" { RET(TK_AMPER_EQUAL) }
            "|=" { RET(TK_VBAR_EQUAL) }
            "^=" { RET(TK_CARET_EQUAL) }
            "<<=" { RET(TK_LEFTSHIFT_EQUAL) }
            ">>=" { RET(TK_RIGHTSHIFT_EQUAL) }
            "**=" { RET(TK_POW_EQUAL) }
            "//=" { RET(TK_DOUBLESLASH_EQUAL) }

            // Relational operators
            "=="   { RET(TK_EQ) }
            '.eq.' { WARN_REL(EQ) RET(TK_EQ) }

            "!="   { RET(TK_NE) }
            '.ne.' { WARN_REL(NE) RET(TK_NE) }

            "<"    { RET(TK_LT) }
            '.lt.' { WARN_REL(LT) RET(TK_LT) }

            "<="   { RET(TK_LE) }
            '.le.' { WARN_REL(LE) RET(TK_LE) }

            ">"    { RET(TK_GT) }
            '.gt.' { WARN_REL(GT) RET(TK_GT) }

            ">="   { RET(TK_GE) }
            '.ge.' { WARN_REL(GE) RET(TK_GE) }

            // Logical operators
            'not'  { RET(TK_NOT) }
            'and'  { RET(TK_AND) }
            'or'   { RET(TK_OR) }
            '.xor.'  { RET(TK_XOR) }
            '.eqv.'  { RET(TK_EQV) }
            '.neqv.' { RET(TK_NEQV) }

            // True/False

            'True' { RET(TK_TRUE) }
            'False' { RET(TK_FALSE) }

            // This is needed to ensure that 2.op.3 gets tokenized as
            // TK_INTEGER(2), TK_DEFOP(.op.), TK_INTEGER(3), and not
            // TK_REAL(2.), TK_NAME(op), TK_REAL(.3). The `.op.` can be a
            // built-in or custom defined operator, such as: `.eq.`, `.not.`,
            // or `.custom.`.
            integer / defop {
                lex_int_large(al, tok, cur,
                    yylval.int_suffix.int_n,
                    yylval.int_suffix.int_kind);
                RET(TK_INTEGER)
            }


            real { token(yylval.string); RET(TK_REAL) }
            integer / (whitespace name) {
                if (last_token == yytokentype::TK_NEWLINE) {
                    uint64_t u;
                    if (lex_int(tok, cur, u, yylval.int_suffix.int_kind)) {
                            yylval.n = u;
                            if (enddo_label_stack[enddo_label_stack.size()-1] == u) {
                                while (enddo_label_stack[enddo_label_stack.size()-1] == u) {
                                    enddo_label_stack.pop_back();
                                    enddo_insert_count++;
                                }
                                enddo_newline_process = true;
                            } else {
                                enddo_newline_process = false;
                            }
                            RET(TK_LABEL)
                    } else {
                        token_loc(loc);
                        std::string t = token();
                        throw LFortran::parser_local::TokenizerError("Integer '" + t + "' too large",
                            loc);
                    }
                } else {
                    lex_int_large(al, tok, cur,
                        yylval.int_suffix.int_n,
                        yylval.int_suffix.int_kind);
                    RET(TK_INTEGER)
                }
            }
            integer {
                lex_int_large(al, tok, cur,
                    yylval.int_suffix.int_n,
                    yylval.int_suffix.int_kind);
                RET(TK_INTEGER)
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

            comment newline {
                line_num++; cur_line=cur;
                token(yylval.string);
                yylval.string.n--;
                token_loc(loc);
                if (last_token == yytokentype::TK_NEWLINE) {
                    return yytokentype::TK_COMMENT;
                } else {
                    last_token=yytokentype::TK_NEWLINE;
                    return yytokentype::TK_EOLCOMMENT;
                }
            }
            //docstring { RET(TK_DOCSTRING) }

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

std::string token(unsigned char *tok, unsigned char* cur)
{
    return std::string((char *)tok, cur - tok);
}

void token_loc(Location &loc)
{
    loc.first = 1;
    loc.last = 1;
}

void lex_format(unsigned char *&cur, Location &loc,
        unsigned char *&start) {
    int num_paren = 0;
    for (;;) {
        unsigned char *tok = cur;
        unsigned char *mar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            int = digit+;
            data_edit_desc
                = 'I' int ('.' int)?
                | 'B' int ('.' int)?
                | 'O' int ('.' int)?
                | 'Z' int ('.' int)?
                | 'F' int '.' int
                | 'E' int '.' int ('E' int)?
                | 'EN' int '.' int ('E' int)?
                | 'ES' int '.' int ('E' int)?
                | 'EX' int '.' int ('E' int)?
                | 'G' int ('.' int ('E' int)?)?
                | 'L' int
                | 'A' (int)?
                | 'D' int '.' int
                | 'PE' int '.' int
                | 'PF' int '.' int
                | 'P'
                | 'X'
                ;
            position_edit_desc
                = 'T' int
                | 'TL' int
                | 'TR' int
                | int 'X'
                ;
            control_edit_desc
                = position_edit_desc
                | (int)? '/'
                | ':'
                ;

            * {
                token_loc(loc);
                std::string t = token(tok, cur);
                throw LFortran::parser_local::TokenizerError("Token '" + t
                    + "' is not recognized in `format` statement", loc);
            }
            '(' {
                if (num_paren == 0) {
                    num_paren++;
                    start = cur;
                    continue;
                } else {
                    cur--;
                    unsigned char *tmp;
                    lex_format(cur, loc, tmp);
                    continue;
                }
            }
            int whitespace? '(' {
                cur--;
                unsigned char *tmp;
                lex_format(cur, loc, tmp);
                continue;
            }
            '*' whitespace? '(' {
                cur--;
                unsigned char *tmp;
                lex_format(cur, loc, tmp);
                continue;
            }
            ')' {
                LFORTRAN_ASSERT(num_paren == 1);
                return;
            }
            end {
                token_loc(loc);
                std::string t = token(tok, cur);
                throw LFortran::parser_local::TokenizerError(
                    "End of file not expected in `format` statement '" + t + "'", loc);
            }
            whitespace { continue; }
            ',' { continue; }
            "&" ws_comment+ whitespace? "&"? { continue; }
            '"' ('""'|[^"\x00])* '"' { continue; }
            "'" ("''"|[^'\x00])* "'" { continue; }
            (int)? data_edit_desc { continue; }
            control_edit_desc { continue; }
        */
    }
}


#define T(tk, name) case (yytokentype::tk) : return name;

std::string token2text(const int token)
{
    if (0 < token && token < 256) {
        char t = token;
        return std::string(&t, 1);
    }
    switch (token) {
        T(END_OF_FILE, "end of file")
        T(TK_NEWLINE, "newline")
        T(TK_INDENT, "indent")
        T(TK_DEDENT, "dedent")
        T(TK_DOCSTRING, "docstring")
        T(TK_NAME, "identifier")
        T(TK_DEF_OP, "defined operator")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_BOZ_CONSTANT, "BOZ constant")

        T(TK_PLUS, "+")
        T(TK_MINUS, "-")
        T(TK_STAR, "*")
        T(TK_SLASH, "/")
        T(TK_BACKSLASH, "\\")
        T(TK_COLON, ":")
        T(TK_SEMICOLON, ";")
        T(TK_COMMA, ",")
        T(TK_EQUAL, "=")
        T(TK_LPAREN, "(")
        T(TK_RPAREN, ")")
        T(TK_LBRACKET, "[")
        T(TK_RBRACKET, "]")
        T(TK_RBRACKET_OLD, "/)")
        T(TK_LBRACE, "{")
        T(TK_RBRACE, "}")
        T(TK_PERCENT, "%")
        T(TK_VBAR, "|")
        T(TK_AMPERSAND, "&")
        T(TK_DOT, ".")
        T(TK_BACKQUOTE, "`")
        T(TK_TILDE, "~")
        T(TK_CARET, "^")
        T(TK_AT, "@")

        T(TK_STRING, "string")
        T(TK_COMMENT, "comment")
        T(TK_EOLCOMMENT, "eolcomment")
        T(TK_LABEL, "label")

        T(TK_DBL_DOT, "..")
        T(TK_DBL_COLON, "::")
        T(TK_POW, "**")
        T(TK_FLOOR_DIV, "//")
        T(TK_ARROW, "=>")
        T(TK_RIGHTSHIFT, ">>")
        T(TK_LEFTSHIFT, "<<")
        T(TK_PLUS_EQUAL, "+=")
        T(TK_MIN_EQUAL, "-=")
        T(TK_STAR_EQUAL, "*=")
        T(TK_SLASH_EQUAL, "/=")
        T(TK_PERCENT_EQUAL, "%=")
        T(TK_AMPER_EQUAL, "&=")
        T(TK_VBAR_EQUAL, "|=")
        T(TK_CARET_EQUAL, "^=")
        T(TK_LEFTSHIFT_EQUAL, "<<=")
        T(TK_RIGHTSHIFT_EQUAL, ">>=")
        T(TK_POW_EQUAL, "**=")
        T(TK_DOUBLESLASH_EQUAL, "//=")

        T(TK_EQ, "==")
        T(TK_NE, "!=")
        T(TK_LT, "<")
        T(TK_LE, "<=")
        T(TK_GT, ">")
        T(TK_GE, ">=")

        T(TK_NOT, "not")
        T(TK_AND, "and")
        T(TK_OR, "or")
        T(TK_EQV, ".eqv.")
        T(TK_NEQV, ".neqv.")

        T(TK_TRUE, "True")
        T(TK_FALSE, "False")

        T(TK_FORMAT, "format")

        T(KW_ABSTRACT, "abstract")
        T(KW_ALL, "all")
        T(KW_ALLOCATABLE, "allocatable")
        T(KW_ALLOCATE, "allocate")
        T(KW_ASSIGN, "assign")
        T(KW_ASSIGNMENT, "assignment")
        T(KW_ASSOCIATE, "associate")
        T(KW_ASYNCHRONOUS, "asynchronous")
        T(KW_BACKSPACE, "backspace")
        T(KW_BIND, "bind")
        T(KW_BLOCK, "block")
        T(KW_CALL, "call")
        T(KW_CASE, "case")
        T(KW_CHANGE, "change")
        T(KW_CHANGE_TEAM, "changeteam")
        T(KW_CHARACTER, "character")
        T(KW_CLASS, "class")
        T(KW_CLOSE, "close")
        T(KW_CODIMENSION, "codimension")
        T(KW_COMMON, "common")
        T(KW_COMPLEX, "complex")
        T(KW_CONCURRENT, "concurrent")
        T(KW_CONTAINS, "contains")
        T(KW_CONTIGUOUS, "contiguous")
        T(KW_CONTINUE, "continue")
        T(KW_CRITICAL, "critical")
        T(KW_CYCLE, "cycle")
        T(KW_DATA, "data")
        T(KW_DEALLOCATE, "deallocate")
        T(KW_DEFAULT, "default")
        T(KW_DEF, "def")
        T(KW_DEFERRED, "deferred")
        T(KW_DIMENSION, "dimension")
        T(KW_DO, "do")
        T(KW_DOWHILE, "dowhile")
        T(KW_DOUBLE, "double")
        T(KW_DOUBLE_PRECISION, "doubleprecision")
        T(KW_ELEMENTAL, "elemental")
        T(KW_ELSE, "else")
        T(KW_ELSEIF, "elseif")
        T(KW_ELSEWHERE, "elsewhere")

        T(KW_END, "end")
        T(KW_END_DO, "end do")
        T(KW_ENDDO, "enddo")
        T(KW_END_IF, "end if")
        T(KW_ENDIF, "endif")
        T(KW_END_INTERFACE, "end interface")
        T(KW_ENDINTERFACE, "endinterface")
        T(KW_END_TYPE, "end type")
        T(KW_ENDTYPE, "endtype")
        T(KW_END_PROGRAM, "end program")
        T(KW_ENDPROGRAM, "endprogram")
        T(KW_END_MODULE, "end module")
        T(KW_ENDMODULE, "endmodule")
        T(KW_END_SUBMODULE, "end submodule")
        T(KW_ENDSUBMODULE, "endsubmodule")
        T(KW_END_BLOCK, "end block")
        T(KW_ENDBLOCK, "endblock")
        T(KW_END_BLOCK_DATA, "end block data")
        T(KW_ENDBLOCKDATA, "endblockdata")
        T(KW_END_SUBROUTINE, "end subroutine")
        T(KW_ENDSUBROUTINE, "endsubroutine")
        T(KW_END_FUNCTION, "end function")
        T(KW_ENDFUNCTION, "endfunction")
        T(KW_END_PROCEDURE, "end procedure")
        T(KW_ENDPROCEDURE, "endprocedure")
        T(KW_END_ENUM, "end enum")
        T(KW_ENDENUM, "endenum")
        T(KW_END_SELECT, "end select")
        T(KW_ENDSELECT, "endselect")
        T(KW_END_ASSOCIATE, "end associate")
        T(KW_ENDASSOCIATE, "endassociate")
        T(KW_END_FORALL, "end forall")
        T(KW_ENDFORALL, "endforall")
        T(KW_END_WHERE, "end where")
        T(KW_ENDWHERE, "endwhere")
        T(KW_END_CRITICAL, "end critical")
        T(KW_ENDCRITICAL, "endcritical")
        T(KW_END_FILE, "end file")
        T(KW_ENDFILE, "endfile")
        T(KW_END_TEAM, "end team")
        T(KW_ENDTEAM, "endteam")

        T(KW_ENTRY, "entry")
        T(KW_ENUM, "enum")
        T(KW_ENUMERATOR, "enumerator")
        T(KW_EQUIVALENCE, "equivalence")
        T(KW_ERRMSG, "errmsg")
        T(KW_ERROR, "error")
        T(KW_EVENT, "event")
        T(KW_EXIT, "exit")
        T(KW_EXTENDS, "extends")
        T(KW_EXTERNAL, "external")
        T(KW_FILE, "file")
        T(KW_FINAL, "final")
        T(KW_FLUSH, "flush")
        T(KW_FORALL, "forall")
        T(KW_FORMATTED, "formatted")
        T(KW_FORM, "form")
        T(KW_FORM_TEAM, "formteam")
        T(KW_FROM, "from")
        T(KW_FUNCTION, "function")
        T(KW_GENERIC, "generic")
        T(KW_GO, "go")
        T(KW_GOTO, "goto")
        T(KW_IF, "if")
        T(KW_IMAGES, "images")
        T(KW_IMPLICIT, "implicit")
        T(KW_IMPORT, "import")
        T(KW_IMPURE, "impure")
        T(KW_IN, "in")
        T(KW_INCLUDE, "include")
        T(KW_INOUT, "inout")
        T(KW_INQUIRE, "inquire")
        T(KW_INTEGER, "integer")
        T(KW_INTENT, "intent")
        T(KW_INTERFACE, "interface")
        T(KW_INTRINSIC, "intrinsic")
        T(KW_IS, "is")
        T(KW_KIND, "kind")
        T(KW_LEN, "len")
        T(KW_LOCAL, "local")
        T(KW_LOCAL_INIT, "local_init")
        T(KW_LOGICAL, "logical")
        T(KW_MEMORY, "memory")
        T(KW_MODULE, "module")
        T(KW_MOLD, "mold")
        T(KW_NAME, "name")
        T(KW_NAMELIST, "namelist")
        T(KW_NEW_INDEX, "new_index")
        T(KW_NOPASS, "nopass")
        T(KW_NON_INTRINSIC, "non_intrinsic")
        T(KW_NON_OVERRIDABLE, "non_overridable")
        T(KW_NON_RECURSIVE, "non_recursive")
        T(KW_NONE, "none")
        T(KW_NULLIFY, "nullify")
        T(KW_ONLY, "only")
        T(KW_OPEN, "open")
        T(KW_OPERATOR, "operator")
        T(KW_OPTIONAL, "optional")
        T(KW_OUT, "out")
        T(KW_PARAMETER, "parameter")
        T(KW_PASS, "pass")
        T(KW_POINTER, "pointer")
        T(KW_POST, "post")
        T(KW_PRECISION, "precision")
        T(KW_PRINT, "print")
        T(KW_PRIVATE, "private")
        T(KW_PROCEDURE, "procedure")
        T(KW_PROGRAM, "program")
        T(KW_PROTECTED, "protected")
        T(KW_PUBLIC, "public")
        T(KW_PURE, "pure")
        T(KW_QUIET, "quiet")
        T(KW_RANK, "rank")
        T(KW_READ, "read")
        T(KW_REAL, "real")
        T(KW_RECURSIVE, "recursive")
        T(KW_REDUCE, "reduce")
        T(KW_RESULT, "result")
        T(KW_RETURN, "return")
        T(KW_REWIND, "rewind")
        T(KW_SAVE, "save")
        T(KW_SELECT, "select")
        T(KW_SELECT_CASE, "selectcase")
        T(KW_SELECT_RANK, "selectrank")
        T(KW_SELECT_TYPE, "selecttype")
        T(KW_SEQUENCE, "sequence")
        T(KW_SHARED, "shared")
        T(KW_SOURCE, "source")
        T(KW_STAT, "stat")
        T(KW_STOP, "stop")
        T(KW_SUBMODULE, "submodule")
        T(KW_SUBROUTINE, "subroutine")
        T(KW_SYNC, "sync")
        T(KW_SYNC_ALL, "syncall")
        T(KW_SYNC_IMAGES, "synimages")
        T(KW_SYNC_MEMORY, "syncmemory")
        T(KW_SYNC_TEAM, "syncteam")
        T(KW_TARGET, "target")
        T(KW_TEAM, "team")
        T(KW_TEAM_NUMBER, "team_number")
        T(KW_THEN, "then")
        T(KW_TO, "to")
        T(KW_TYPE, "type")
        T(KW_UNFORMATTED, "unformatted")
        T(KW_USE, "use")
        T(KW_VALUE, "value")
        T(KW_VOLATILE, "volatile")
        T(KW_WAIT, "wait")
        T(KW_WHERE, "where")
        T(KW_WHILE, "while")
        T(KW_WRITE, "write")
        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw LFortranException("Token conversion not implemented yet.");
        }
    }
}

Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes,
        std::vector<Location> *locations)
{
    Tokenizer t;
    t.set_string(input);
    std::vector<int> tst;
    int token = yytokentype::END_OF_FILE + 1; // Something different from EOF
    while (token != yytokentype::END_OF_FILE) {
        YYSTYPE y;
        Location l;
        try {
            token = t.lex(al, y, l, diagnostics);
        } catch (const parser_local::TokenizerError &e) {
            diagnostics.diagnostics.push_back(e.d);
            return Error();
        }
        tst.push_back(token);
        if (stypes) stypes->push_back(y);
        if (locations) locations->push_back(l);
    }
    return tst;
}

std::string pickle_token(int token, const LFortran::YYSTYPE &yystype)
{
    std::string t;
    t += "(";
    if ((token >= yytokentype::TK_NAME && token <= TK_FALSE) ||
            (token >= yytokentype::TK_DOCSTRING && token <= TK_DEDENT)) {
        t += "TOKEN";
    } else if (token == yytokentype::TK_NEWLINE) {
        t += "NEWLINE";
        t += ")";
        return t;
    } else if (token == yytokentype::END_OF_FILE) {
        t += "EOF";
        t += ")";
        return t;
    } else {
        t += "KEYWORD";
    }
    t += " \"";
    t += token2text(token);
    t += "\"";
    if (token == yytokentype::TK_NAME) {
        t += " " + yystype.string.str();
    } else if (token == yytokentype::TK_INTEGER) {
        t += " " + yystype.int_suffix.int_n.str();
        if (yystype.int_suffix.int_kind.p) {
            t += "_" + yystype.int_suffix.int_kind.str();
        }
    } else if (token == yytokentype::TK_STRING) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    } else if (token == yytokentype::TK_BOZ_CONSTANT) {
        t += " " + yystype.string.str();
    }
    t += ")";
    return t;
}


} // namespace LFortran
