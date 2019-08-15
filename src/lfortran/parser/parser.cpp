#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

namespace LFortran
{

LFortran::AST::expr_t *parse(Allocator &al, const std::string &s)
{
    Parser p(al);
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
