#ifndef LPYTHON_SRC_PARSER_TOKENIZER_H
#define LPYTHON_SRC_PARSER_TOKENIZER_H
#include <libasr/exception.h>
#include <libasr/alloc.h>
#include <lpython/parser/parser_stype.h>

#define MAX_PAREN_LEVEL 200

namespace LFortran
{

class Tokenizer
{
public:
    unsigned char *cur;
    unsigned char *tok;
    unsigned char *cur_line;
    unsigned int line_num;
    unsigned char *string_start;

    int last_token=-1;

    bool indent = false; // Next line is expected to be indented
    int dedent = 0; // Allowed values: 0, 1, 2, see the code below the meaning of this state variable
    bool colon_actual_last_token = false; // If the actual last token was a colon
    long int last_indent_length = 0;
    std::vector<uint64_t> indent_length;

    char paren_stack[MAX_PAREN_LEVEL];
    size_t parenlevel = 0;

public:
    // Set the string to tokenize. The caller must ensure `str` will stay valid
    // as long as `lex` is being called.
    void set_string(const std::string &str);

    // Get next token. Token ID is returned as function result, the semantic
    // value is put into `yylval`.
    int lex(Allocator &al, YYSTYPE &yylval, Location &loc, diag::Diagnostics &diagnostics);

    // Return the current token as std::string
    std::string token() const
    {
        return std::string((char *)tok, cur - tok);
    }

    // Return the current token as YYSTYPE::Str
    void token(Str &s) const
    {
        s.p = (char*) tok;
        s.n = cur-tok;
    }

    // Return the current token as YYSTYPE::Str, strips first and last character
    void token_str(Str &s) const
    {
        s.p = (char*) tok + 1;
        s.n = cur-tok-2;
    }

    // Return the current token as YYSTYPE::Str, strips the first 3 and the last
    // 3 characters
    void token_str3(Str &s) const
    {
        s.p = (char*) tok + 3;
        s.n = cur-tok-6;
    }

    // Return the current token's location
    void token_loc(Location &loc) const
    {
        loc.first = tok-string_start;
        loc.last = cur-string_start-1;
    }

    void record_paren(Location &loc, char c);
};

std::string token2text(const int token);

// Tokenizes the `input` and return a list of tokens
Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes=nullptr,
        std::vector<Location> *locations=nullptr);

std::string pickle_token(int token, const YYSTYPE &yystype);

std::string get_token_name(int token, const LFortran::YYSTYPE &yystype);

} // namespace LFortran

#endif // LPYTHON_SRC_PARSER_TOKENIZER_H
