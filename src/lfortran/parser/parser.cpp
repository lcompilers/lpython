#include <lfortran/parser/parser.h>
#include <lfortran/parser/parser.tab.hh>

namespace LFortran
{

LFortran::AST::ast_t *parse(Allocator &al, const std::string &s)
{
    Parser p(al);
    p.parse(s);
    return p.result[0];
}

std::vector<LFortran::AST::ast_t*> parsen(Allocator &al, const std::string &s)
{
    Parser p(al);
    p.parse(s);
    return p.result;
}

void Parser::parse(const std::string &input)
{
    inp = input;
    m_tokenizer.set_string(inp);
    if (yyparse(*this) == 0) {
        LFORTRAN_ASSERT(result.size() >= 1);
        return;
    }
    throw ParserError("Parsing Unsuccessful");
}

}
