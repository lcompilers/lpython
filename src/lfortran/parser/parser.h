#ifndef LFORTRAN_PARSER_PARSER_H
#define LFORTRAN_PARSER_PARSER_H

#include <fstream>
#include <algorithm>
#include <memory>

#include <lfortran/parser/tokenizer.h>

namespace LFortran
{

class Parser
{
    std::string inp;

public:
    Allocator &m_a;
    Tokenizer m_tokenizer;
    LFortran::AST::ast_t *result;

    Parser(Allocator &al) : m_a{al} {}
    void parse(const std::string &input);
    int parse();


private:
};

LFortran::AST::ast_t *parse(Allocator &al, const std::string &s);

} // namespace LFortran

#endif
