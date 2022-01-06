#ifndef LFORTRAN_PARSER_PARSER_H
#define LFORTRAN_PARSER_PARSER_H

#include <fstream>
#include <algorithm>
#include <memory>

#include <libasr/containers.h>
#include <libasr/diagnostics.h>
#include <lfortran/parser/tokenizer.h>

namespace LFortran
{

class Parser
{
public:
    std::string inp;

public:
    diag::Diagnostics &diag;
    Allocator &m_a;
    Tokenizer m_tokenizer;
    Vec<AST::ast_t*> result;

    Parser(Allocator &al, diag::Diagnostics &diagnostics)
            : diag{diagnostics}, m_a{al} {
        result.reserve(al, 32);
    }

    void parse(const std::string &input);
    void handle_yyerror(const Location &loc, const std::string &msg);
};


// Parses Fortran code to AST
Result<AST::TranslationUnit_t*> parse(Allocator &al,
    const std::string &s,
    diag::Diagnostics &diagnostics);

// Tokenizes the `input` and return a list of tokens
Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes=nullptr,
        std::vector<Location> *locations=nullptr);

// Converts token number to text
std::string token2text(const int token);

std::string fix_continuation(const std::string &s, LocationManager &lm,
        bool fixed_form);

} // namespace LFortran

#endif
