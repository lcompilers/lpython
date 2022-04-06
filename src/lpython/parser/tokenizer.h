#ifndef LPYTHON_SRC_PARSER_TOKENIZER_H
#define LPYTHON_SRC_PARSER_TOKENIZER_H

#include <libasr/exception.h>
#include <libasr/alloc.h>
#include <lpython/parser/parser_stype.h>

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

    std::vector<uint64_t> enddo_label_stack = {0};
    bool enddo_newline_process = false;
    int enddo_state = 0;
    int enddo_insert_count = 0;
    bool indent = false; // Next line is expected to be indented
    int dedent = 0; // Allowed values: 0, 1, 2, see the code below the meaning of this state variable
    long int last_indent_length = 0;
    std::vector<uint64_t> indent_length;

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

    // Return the current token's location
    void token_loc(Location &loc) const
    {
        loc.first = tok-string_start;
        loc.last = cur-string_start-1;
    }
    void add_rel_warning(diag::Diagnostics &diagnostics, int rel_token) const;
};

bool lex_int(const unsigned char *s, const unsigned char *e, uint64_t &u,
    Str &suffix);

std::string token2text(const int token);

// Tokenizes the `input` and return a list of tokens
Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes=nullptr,
        std::vector<Location> *locations=nullptr);

std::string pickle_token(int token, const YYSTYPE &yystype);


} // namespace LFortran

#endif // LPYTHON_SRC_PARSER_TOKENIZER_H
