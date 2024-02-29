#include <limits>
#include <iostream>
#include <lpython/parser/parser_exception.h>
#include <lpython/parser/tokenizer.h>
#include <lpython/parser/parser.tab.hh>
#include <libasr/bigint.h>

namespace LCompilers::LPython {

template<int base>
bool adddgt(uint64_t &u, uint64_t d)
{
    if (u > (std::numeric_limits<uint64_t>::max() - d) / base) {
        return false;
    }
    u = u * base + d;
    return true;
}

bool lex_oct(const unsigned char *s, const unsigned char *e, uint64_t &u)
{
    for (u = 0, ++s; s < e; ++s) {
        if (!adddgt<8>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

bool lex_dec(const unsigned char *s, const unsigned char *e, uint64_t &u)
{
    for (u = 0; s < e; ++s) {
        if (*s == '_') continue;
        if (!adddgt<10>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

void lex_dec_int_large(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u)
{
    uint64_t ui;
    if (lex_dec(s, e, ui)) {
        if (ui <= BigInt::MAX_SMALL_INT) {
            u.from_smallint(ui);
            return;
        }
    }
    // TODO: remove underscore from large ints
    const unsigned char *start = s;
    Str num;
    num.p = (char*)start;
    num.n = e-start;
    u.from_largeint(al, num);
}

const std::string remove_underscore(std::string s) {
    s.erase(remove(s.begin(), s.end(), '_'), s.end());
    return s;
}

uint64_t get_value(char *s, int base, const Location &loc) {
    std::string str(remove_underscore(s));
    uint64_t n;
    try {
        n = std::stoull(str, nullptr, base);
    } catch (const std::out_of_range &e) {
        throw parser_local::TokenizerError(
            "Integer too large to convert", {loc});
    }
    return n;
}

// Converts integer (Dec, Hex, Bin, Oct) from a string to BigInt `u`
// s ... the start of the integer
// e ... the character after the end
void lex_int(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u, const Location &loc)
{
    if (std::tolower(s[1]) == 'x') {
        // Hex
        s = s + 2;
        uint64_t n = get_value((char*)s, 16, loc);
        u.from_smallint(n);
    } else if (std::tolower(s[1]) == 'b') {
        // Bin
        s = s + 2;
        uint64_t n = get_value((char*)s, 2, loc);
        u.from_smallint(n);
    } else if ((std::tolower(s[1]) == 'o')) {
        // Oct
        s = s + 2;
        uint64_t n = get_value((char*)s, 8, loc);
        u.from_smallint(n);
    } else {
        lex_dec_int_large(al, s, e, u);
    }
    return;
}

// Tokenizes imag num of value 123j into `u` and `suffix`
// s ... the start of the integer
// e ... the character after the end
void lex_imag(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u, Str &suffix)
{
    const unsigned char *start = s;
    for (; s < e; ++s) {
        if ((*s == 'j') | (*s == 'J')) {
            suffix.p = (char*) s; // `j`
            suffix.n = 1;

            Str num;
            num.p = (char*)start;
            num.n = s-start;
            u.from_largeint(al, num);
            return;
        }
    }
}

void Tokenizer::set_string(const std::string &str, uint32_t prev_loc_)
{
    // The input string must be NULL terminated, otherwise the tokenizer will
    // not detect the end of string. After C++11, the std::string is guaranteed
    // to end with \0, but we check this here just in case.
    LCOMPILERS_ASSERT(str[str.size()] == '\0');
    cur = (unsigned char *)(&str[0]);
    string_start = cur;
    prev_loc = prev_loc_;
    cur_line = cur;
    line_num = 1;
}

void Tokenizer::record_paren(Location &loc, char c) {
    switch (c) {
        case '(':
        case '[':
        case '{':
            if(parenlevel >= MAX_PAREN_LEVEL) {
                throw parser_local::TokenizerError(
                    "Too many nested parentheses", {loc});
            }
            paren_stack[parenlevel] = c;
            parenlevel++;
            break;

        case ')':
        case ']':
        case '}':
            if(parenlevel < 1) {
                throw parser_local::TokenizerError(
                    "Parenthesis unexpected", {loc});
            }
            parenlevel--;

            char prev_paren = paren_stack[parenlevel];
            if(!((prev_paren == '(' && c == ')') ||
                 (prev_paren == '[' && c == ']') ||
                 (prev_paren == '{' && c == '}'))) {
                throw parser_local::TokenizerError(
                    "Parentheses does not match", {loc});
            }
            break;
    }
    return;
}

#define KW(x) token(yylval.string); RET(KW_##x);
#define RET(x) token_loc(loc); last_token=yytokentype::x; return yytokentype::x;

int Tokenizer::lex(Allocator &al, YYSTYPE &yylval, Location &loc, diag::Diagnostics &/*diagnostics*/)
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
        if((long int)indent_length.back() != last_indent_length) {
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
        unsigned char *mar; //, *ctxmar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            // re2c:define:YYCTXMARKER = ctxmar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            whitespace = [ \t\v]+;
            newline = "\n" | "\r\n";
            digit = [0-9];
            int_oct = "0"[oO]([0-7] | "_" [0-7])+;
            int_bin = "0"[bB]([01] | "_" [01])+;
            int_hex = "0"[xX]([0-9a-fA-F] | "_" [0-9a-fA-F])+;
            digits = digit+ (digit | "_" digit)*;
            char =  [^\x00-\x7F]|[a-zA-Z_];
            name = char (char | digit)*;
            significand = (digits "." digits?) | ("." digits);
            exp = [eE][-+]? digits;
            integer = digits | int_oct | int_bin | int_hex;
            real = (significand exp?) | (digits exp);
            imag_number = (real | digits)[jJ];
            string1 = '"' ('\\'[^\x00] | [^"\x00\n\\])* '"';
            string2 = "'" ("\\"[^\x00] | [^'\x00\n\\])* "'";
            string3 = '"""' ( '\\'[^\x00]
                            | ('"' | '"' '\\'+ '"' | '"' '\\'+) [^"\x00\\]
                            | ('""' | '""' '\\'+) [^"\x00\\]
                            | [^"\x00\\] )*
                      '"""';
            string4 = "'''" ( "\\"[^\x00]
                            | ("'" | "'" "\\"+ "'" | "'" "\\"+) [^'\x00\\]
                            | ("''" | "''" "\\"+) [^'\x00\\]
                            | [^'\x00\\] )*
                      "'''";
            
            fstring_start = ([fF] | [fF][rR] | [rR][fF]) '"' ('\\'[^\x00{}] | [^"\x00\n\\{}])* '{';
            fstring_middle = '}' ('\\'[^\x00{}] | [^"\x00\n\\{}])* '{';
            fstring_end = '}' ('\\'[^\x00{}] | [^"\x00\n\\{}])* '"';

            type_ignore = "#" whitespace? "type:" whitespace? "ignore" [^\n\x00]*;
            type_comment = "#" whitespace? "type:" whitespace? [^\n\x00]*;
            comment = "#" [^\n\x00]*;
            // docstring = newline whitespace? string1 | string2;
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
            end {
                token_loc(loc);
                if(parenlevel) {
                    throw parser_local::TokenizerError(
                        "Parentheses was never closed", {loc});
                }
                RET(END_OF_FILE);
            }

            whitespace {
                if(cur[0] == '#') { continue; }
                if(last_token == yytokentype::TK_NEWLINE && cur[0] == '\n') {
                    continue;
                }
                if (indent) {
                    indent = false;
                    if (cur[0] != ' ' && cur[0] != '\t'
                        && last_indent_length < cur-tok) {
                        if (last_indent_length == 0) {
                            last_indent_type = tok[0];
                        }
                        if (last_indent_type == tok[0]) {
                            indent_length.push_back(cur-tok);
                            last_indent_length = cur-tok;
                            RET(TK_INDENT);
                        } else {
                            token_loc(loc);
                            throw parser_local::TokenizerError(
                            "Indentation should be of the same type "
                            "(either tabs or spaces)", {loc});
                        }
                    } else {
                        token_loc(loc);
                        throw parser_local::TokenizerError(
                        "Expected an indented block.", {loc});
                    }
                } else {
                    if(last_token == yytokentype::TK_NEWLINE
                            && cur[0] != ' ' && cur[0] != '\t') {
                        if (last_indent_type == tok[0]) {
                            if (last_indent_length > cur-tok) {
                                last_indent_length = cur-tok;
                                dedent = 2;
                                if (!indent_length.empty()) {
                                    indent_length.pop_back();
                                }
                                RET(TK_DEDENT);
                            }
                        } else {
                            token_loc(loc);
                            throw parser_local::TokenizerError(
                            "Indentation should be of the same type "
                            "(either tabs or spaces)", {loc});
                        }
                    }
                }
                continue;
             }

            // Keywords
            "as"       { KW(AS) }
            "assert"   { KW(ASSERT) }
            "async"    { KW(ASYNC) }
            "await"    { KW(AWAIT) }
            "break"    { KW(BREAK) }
            "class"    { KW(CLASS) }
            "continue" { KW(CONTINUE) }
            "def"      { KW(DEF) }
            "del"      { KW(DEL) }
            "elif"     { KW(ELIF) }
            "else"     { KW(ELSE) }
            "except"   { KW(EXCEPT) }
            "finally"  { KW(FINALLY) }
            "for"      { KW(FOR) }
            "from"     { KW(FROM) }
            "global"   { KW(GLOBAL) }
            "if"       { KW(IF) }
            "import"   { KW(IMPORT) }
            "in"       { KW(IN) }
            "is"       { KW(IS) }
            "lambda"   { KW(LAMBDA) }
            "None"     { KW(NONE) }
            "nonlocal" { KW(NONLOCAL) }
            "pass"     { KW(PASS) }
            "raise"    { KW(RAISE) }
            "return"   { KW(RETURN) }
            "try"      { KW(TRY) }
            "while"    { KW(WHILE) }
            "with"     { KW(WITH) }
            "yield"    { KW(YIELD) }
            "yield" whitespace "from" whitespace { KW(YIELD_FROM) }

            // Soft Keywords
            "match" / [^:\n\x00] {
                if ((last_token == -1
                  || last_token == yytokentype::TK_DEDENT
                  || last_token == yytokentype::TK_INDENT
                  || last_token == yytokentype::TK_NEWLINE)
                  && parenlevel == 0) {
                    bool is_match_keyword = false;
                    lex_match_or_case(loc, cur, is_match_keyword);
                    if (is_match_keyword) {
                        KW(MATCH);
                    } else {
                        token(yylval.string);
                        RET(TK_NAME);
                    }
                } else {
                    token(yylval.string);
                    RET(TK_NAME);
                }
            }
            "case" / [^:\n\x00] {
                if ((last_token == yytokentype::TK_INDENT
                  || last_token == yytokentype::TK_DEDENT)
                  && parenlevel == 0) {
                    bool is_case_keyword = false;
                    lex_match_or_case(loc, cur, is_case_keyword);
                    if (is_case_keyword) {
                        KW(CASE);
                    } else {
                        token(yylval.string);
                        RET(TK_NAME);
                    }
                } else {
                    token(yylval.string);
                    RET(TK_NAME);
                }
            }
            [rR][bB] | [bB][rR]
            | [rR] | [bB] | [uU]
             {
                if(cur[0] == '\'' || cur[0] == '"'){
                    KW(STR_PREFIX);
                }
                else {
                    token(yylval.string);
                    RET(TK_NAME);
                }
            }

            // Tokens
            newline {
                if(parenlevel) { continue; }
                if(cur[0] == '#') { RET(TK_NEWLINE); }
                if (last_token == yytokentype::TK_COLON
                        || colon_actual_last_token) {
                    colon_actual_last_token = false;
                    indent = true;
                } else if (cur[0] != ' ' && cur[0] != '\t' && cur[0] != '\n'
                        && last_indent_length >= cur-tok) {
                    last_indent_length = 0;
                    dedent = 1;
                }
                RET(TK_NEWLINE);
            }

            "\\" newline { continue; }

            // Single character symbols
            "(" { token_loc(loc); record_paren(loc, '('); RET(TK_LPAREN) }
            "[" { token_loc(loc); record_paren(loc, '['); RET(TK_LBRACKET) }
            "{" { token_loc(loc); record_paren(loc, '{'); RET(TK_LBRACE) }
            ")" { token_loc(loc); record_paren(loc, ')'); RET(TK_RPAREN) }
            "]" { token_loc(loc); record_paren(loc, ']'); RET(TK_RBRACKET) }
            "}" { token_loc(loc); record_paren(loc, '}'); RET(TK_RBRACE) }
            "+" { RET(TK_PLUS) }
            "-" { RET(TK_MINUS) }
            "=" { RET(TK_EQUAL) }
            ":" {
                    if(cur[0] == '\n' && !parenlevel){
                        colon_actual_last_token = true;
                    }
                    RET(TK_COLON);
                }
            ";" { RET(TK_SEMICOLON) }
            "/" { RET(TK_SLASH) }
            "%" { RET(TK_PERCENT) }
            "," { RET(TK_COMMA) }
            "*" { RET(TK_STAR) }
            "|" { RET(TK_VBAR) }
            "&" { RET(TK_AMPERSAND) }
            "." { RET(TK_DOT) }
            "~" { RET(TK_TILDE) }
            "^" { RET(TK_CARET) }
            "@" { RET(TK_AT) }

            // Multiple character symbols
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
            "@=" { RET(TK_ATEQUAL) }
            "->" { RET(TK_RARROW) }
            ":=" { RET(TK_COLONEQUAL) }
            "..." { RET(TK_ELLIPSIS) }
            "<<=" { RET(TK_LEFTSHIFT_EQUAL) }
            ">>=" { RET(TK_RIGHTSHIFT_EQUAL) }
            "**=" { RET(TK_POW_EQUAL) }
            "//=" { RET(TK_DOUBLESLASH_EQUAL) }

            // Relational operators
            "=="   { RET(TK_EQ) }
            "!="   { RET(TK_NE) }
            "<"    { RET(TK_LT) }
            "<="   { RET(TK_LE) }
            ">"    { RET(TK_GT) }
            ">="   { RET(TK_GE) }

            // Logical operators
            "not"  { RET(TK_NOT) }
            "and"  { RET(TK_AND) }
            "or"   { RET(TK_OR) }
            "is" whitespace "not" whitespace { RET(TK_IS_NOT) }
            "is" whitespace? "\\" newline whitespace? "not" whitespace { RET(TK_IS_NOT) }
            "not" whitespace "in" "\\" newline { RET(TK_NOT_IN) }
            "not" whitespace "in" whitespace { RET(TK_NOT_IN) }
            "not" whitespace "in" newline { RET(TK_NOT_IN) }
            "not" whitespace? "\\" newline whitespace? "in" "\\" newline { RET(TK_NOT_IN) }
            "not" whitespace? "\\" newline whitespace? "in" whitespace { RET(TK_NOT_IN) }

            // True/False

            "True" { RET(TK_TRUE) }
            "False" { RET(TK_FALSE) }

            real {
                yylval.f = std::strtod(remove_underscore(token()).c_str(), 0);
                RET(TK_REAL)
            }
            integer {
                BigInt::BigInt n;
                token_loc(loc);
                lex_int(al, tok, cur, n, loc);
                yylval.n = n.n;
                RET(TK_INTEGER)
            }
            imag_number {
                yylval.f = std::strtod(remove_underscore(token()).c_str(), 0);
                RET(TK_IMAG_NUM)
            }

            type_ignore {
                if (last_token == yytokentype::TK_COLON && !parenlevel) {
                    indent = true;
                }
                token(yylval.string);
                last_token=yytokentype::TK_NEWLINE;
                return yytokentype::TK_TYPE_IGNORE;
            }

            type_comment {
                if (last_token == yytokentype::TK_COLON && !parenlevel) {
                    indent = true;
                }
                token(yylval.string);
                last_token=yytokentype::TK_NEWLINE;
                return yytokentype::TK_TYPE_COMMENT;
            }

            comment {
                if(last_token == -1) { RET(TK_COMMENT); }
                if(parenlevel) { continue; }
                line_num++; cur_line=cur;
                token(yylval.string);
                // This is commented out because
                // the last character in the comment was skipped.
                // yylval.string.n--;
                token_loc(loc);
                if (last_token == yytokentype::TK_NEWLINE) {
                    return yytokentype::TK_COMMENT;
                } else {
                    if (last_token == yytokentype::TK_COLON) {
                        indent = true;
                    }
                    last_token=yytokentype::TK_NEWLINE;
                    return yytokentype::TK_EOLCOMMENT;
                }
            }
            //docstring { RET(TK_DOCSTRING) }

            string1 { token_str(yylval.string); RET(TK_STRING) }
            string2 { token_str(yylval.string); RET(TK_STRING) }
            string3 { token_str3(yylval.string); RET(TK_STRING) }
            string4 { token_str3(yylval.string); RET(TK_STRING) }

            fstring_start { token(yylval.string); RET(TK_FSTRING_START) }
            fstring_middle { token_str(yylval.string); RET(TK_FSTRING_MIDDLE) }
            fstring_end { token_str(yylval.string); RET(TK_FSTRING_END) }

            name { token(yylval.string); RET(TK_NAME) }
        */
    }
}

void Tokenizer::lex_match_or_case(Location &loc, unsigned char *cur,
        bool &is_match_or_case_keyword) {
    for (;;) {
        unsigned char *tok = cur;
        unsigned char *mar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            * {
                token_loc(loc);
                std::string t = std::string((char *)tok, cur - tok);
                throw parser_local::TokenizerError("Token '"
                    + t + "' is not recognized in `match` statement", loc);
            }

            end {
                token_loc(loc);
                std::string t = std::string((char *)tok, cur - tok);
                throw parser_local::TokenizerError(
                    "End of file not expected within `match` statement: '" + t
                    + "'", loc);
            }

            "(" { record_paren(loc, '('); continue; }
            "[" { record_paren(loc, '['); continue; }
            "{" { record_paren(loc, '{'); continue; }
            ")" { record_paren(loc, ')'); continue; }
            "]" { record_paren(loc, ']'); continue; }
            "}" { record_paren(loc, '}'); continue; }

            ":" newline | ":" whitespace? comment {
                if (parenlevel == 0) {
                    is_match_or_case_keyword = true;
                    return;
                }
                continue;
            }

            "\\" newline { continue; }

            newline {
                if (parenlevel == 0) { return; }
                continue;
            }

            [^\x00] { continue; }
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
        T(TK_NAME, "identifier")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_IMAG_NUM, "imag number")

        T(TK_PLUS, "+")
        T(TK_MINUS, "-")
        T(TK_STAR, "*")
        T(TK_SLASH, "/")
        T(TK_COLON, ":")
        T(TK_SEMICOLON, ";")
        T(TK_COMMA, ",")
        T(TK_EQUAL, "=")
        T(TK_LPAREN, "(")
        T(TK_RPAREN, ")")
        T(TK_LBRACKET, "[")
        T(TK_RBRACKET, "]")
        T(TK_LBRACE, "{")
        T(TK_RBRACE, "}")
        T(TK_PERCENT, "%")
        T(TK_VBAR, "|")
        T(TK_AMPERSAND, "&")
        T(TK_DOT, ".")
        T(TK_TILDE, "~")
        T(TK_CARET, "^")
        T(TK_AT, "@")

        T(TK_STRING, "string")
        T(TK_FSTRING_START, "fstring_start")
        T(TK_FSTRING_MIDDLE, "fstring_middle")
        T(TK_FSTRING_END, "fstring_end")
        T(TK_COMMENT, "comment")
        T(TK_EOLCOMMENT, "eolcomment")
        T(TK_TYPE_COMMENT, "type_comment")
        T(TK_TYPE_IGNORE, "type_ignore")

        T(TK_POW, "**")
        T(TK_FLOOR_DIV, "//")
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
        T(TK_ATEQUAL, "@=")
        T(TK_RARROW, "->")
        T(TK_COLONEQUAL, ":=")
        T(TK_ELLIPSIS, "...")
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
        T(TK_IS_NOT, "is not")
        T(TK_NOT_IN, "not in")

        T(TK_TRUE, "True")
        T(TK_FALSE, "False")

        T(KW_AS, "as")
        T(KW_ASSERT, "assert")
        T(KW_ASYNC, "async")
        T(KW_AWAIT, "await")
        T(KW_BREAK, "break")
        T(KW_CLASS, "class")
        T(KW_CONTINUE, "continue")
        T(KW_DEF, "def")
        T(KW_DEL, "del")
        T(KW_ELIF, "elif")
        T(KW_ELSE, "else")
        T(KW_EXCEPT, "except")
        T(KW_FINALLY, "finally")
        T(KW_FOR, "for")
        T(KW_FROM, "from")
        T(KW_GLOBAL, "global")
        T(KW_IF, "if")
        T(KW_IMPORT, "import")
        T(KW_IN, "in")
        T(KW_IS, "is")
        T(KW_LAMBDA, "lambda")
        T(KW_NONE, "none")
        T(KW_NONLOCAL, "nonlocal")
        T(KW_PASS, "pass")
        T(KW_RAISE, "raise")
        T(KW_RETURN, "return")
        T(KW_TRY, "try")
        T(KW_WHILE, "while")
        T(KW_WITH, "with")
        T(KW_YIELD, "yield")
        T(KW_YIELD_FROM, "yield from")

        T(KW_MATCH, "match")
        T(KW_CASE, "case")
        T(KW_STR_PREFIX, "string prefix")

        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw LCompilersException("Token conversion not implemented yet.");
        }
    }
}

Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes,
        std::vector<Location> *locations)
{
    Tokenizer t;
    t.set_string(input, 0);
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

std::string pickle_token(int token, const YYSTYPE &yystype)
{
    std::string t;
    t += "(";
    if (token >= yytokentype::TK_INDENT && token <= TK_FALSE) {
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
        t += " " + BigInt::int_to_str(yystype.n);
    } else if (token == yytokentype::TK_REAL) {
        t += " " + std::to_string(yystype.f);
    } else if (token == yytokentype::TK_IMAG_NUM) {
        t += " " + std::to_string(yystype.f) + "j";
    } else if (token == yytokentype::TK_STRING) {
        t = t + " " + "\"" + str_escape_c(yystype.string.str()) + "\"";
    } else if (token == yytokentype::TK_TYPE_COMMENT) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    } else if (token == yytokentype::TK_TYPE_IGNORE) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    }
    t += ")";
    return t;
}


} // namespace LCompilers::LPython
