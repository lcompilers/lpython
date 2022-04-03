#include <limits>
#include <iostream>
#include <lpython/parser/parser_exception.h>
#include <lpython/parser/tokenizer.h>
#include <lpython/parser/parser.tab.hh>
#include <libasr/bigint.h>

namespace LFortran
{

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

#define KW(x) token(yylval.string); RET(KW_##x);
#define RET(x) token_loc(loc); last_token=yytokentype::x; return yytokentype::x;


int Tokenizer::lex(Allocator &/*al*/, YYSTYPE &yylval, Location &loc, diag::Diagnostics &/*diagnostics*/)
{
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
        unsigned char *mar;//, *ctxmar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            // re2c:define:YYCTXMARKER = ctxmar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";

            end = "\x00";
            whitespace = [ \t\v\r]+;
            newline = "\n";
            digit = [0-9];
            oct_digit = [0][oO][0-7]+;
            bin_digit = [0][bB][01]+;
            hex_digit = ([0][xX] | [0])[0-9a-fA-F]+;
            char =  [a-zA-Z_];
            name = char (char | digit)*;
            significand = (digit+"."digit*) | ("."digit+);
            exp = [edED][-+]? digit+;
            integer = digit+ | oct_digit | bin_digit | hex_digit;
            real = (significand exp?) | (digit+ exp);
            imag_number = (real | digit+)[jJ];
            string1 = '"' ('""'|[^"\x00])* '"';
            string2 = "'" ("''"|[^'\x00])* "'";
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
            end { RET(END_OF_FILE); }
            whitespace { continue; }

            // Keywords
            'as'       { KW(AS) }
            'assert'   { KW(ASSERT) }
            'async'    { KW(ASYNC) }
            'await'    { KW(AWAIT) }
            'break'    { KW(BREAK) }
            'class'    { KW(CLASS) }
            'continue' { KW(CONTINUE) }
            'def'      { KW(DEF) }
            'del'      { KW(DEL) }
            'elif'     { KW(ELIF) }
            'else'     { KW(ELSE) }
            'except'   { KW(EXCEPT) }
            // 'exec'     { KW(EXEC) }
            'finally'  { KW(FINALLY) }
            'for'      { KW(FOR) }
            'from'     { KW(FROM) }
            'global'   { KW(GLOBAL) }
            'if'       { KW(IF) }
            'import'   { KW(IMPORT) }
            'in'       { KW(IN) }
            'is'       { KW(IS) }
            'lambda'   { KW(LAMBDA) }
            'nonlocal' { KW(NONLOCAL) }
            'pass'     { KW(PASS) }
            // 'print'    { KW(PRINT) }
            'raise'    { KW(RAISE) }
            'return'   { KW(RETURN) }
            'try'      { KW(TRY) }
            'while'    { KW(WHILE) }
            'with'     { KW(WITH) }
            'yield'    { KW(YIELD) }

            // Tokens
            newline {
                RET(TK_NEWLINE)
            }

            "\\" newline { continue; }

            // Single character symbols
            "(" { RET(TK_LPAREN) }
            ")" { RET(TK_RPAREN) }
            "[" { RET(TK_LBRACKET) }
            "]" { RET(TK_RBRACKET) }
            "{" { RET(TK_LBRACE) }
            "}" { RET(TK_RBRACE) }
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
            'not'  { RET(TK_NOT) }
            'and'  { RET(TK_AND) }
            'or'   { RET(TK_OR) }

            // True/False

            'True' { RET(TK_TRUE) }
            'False' { RET(TK_FALSE) }
            'None' { RET(TK_NONE) }

            real { token(yylval.string); RET(TK_REAL) }
            integer { token(yylval.string);  RET(TK_INTEGER) }
            imag_number { token(yylval.string);  RET(TK_IMAG_NUM) }

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

            string1 { token_str(yylval.string); RET(TK_STRING) }
            string2 { token_str(yylval.string); RET(TK_STRING) }

            name { token(yylval.string); RET(TK_NAME) }
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
        T(TK_COMMENT, "comment")
        T(TK_EOLCOMMENT, "eolcomment")

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

        T(TK_TRUE, "True")
        T(TK_FALSE, "False")
        T(TK_NONE, "None")

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
        T(KW_NONLOCAL, "nonlocal")
        T(KW_PASS, "pass")
        T(KW_RAISE, "raise")
        T(KW_RETURN, "return")
        T(KW_TRY, "try")
        T(KW_WHILE, "while")
        T(KW_WITH, "with")
        T(KW_YIELD, "yield")

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
    if (token >= yytokentype::TK_NAME && token <= TK_NONE) {
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
        t += " " + yystype.string.str();
    } else if (token == yytokentype::TK_REAL) {
        t += " " + yystype.string.str();
    } else if (token == yytokentype::TK_IMAG_NUM) {
        t += " " + yystype.string.str();
    } else if (token == yytokentype::TK_STRING) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    }
    t += ")";
    return t;
}


} // namespace LFortran
