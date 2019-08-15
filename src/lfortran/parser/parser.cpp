#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

namespace LFortran
{

Allocator al(1000000000);

LFortran::AST::expr_t *parse(const std::string &s)
{
    Parser p;
    p.parse(s);
    return p.result;
}

void Parser::parse(const std::string &input)
{
    inp = input;
    m_tokenizer.set_string(inp);
    if (yyparse(*this) == 0) return;
    throw ParserError("Parsing Unsuccessful");
}

}
