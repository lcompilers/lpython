#include <string>
#include <sstream>

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
    Location loc;
    throw ParserError("Parsing Unsuccessful", loc, 0);
}

std::string get_line(std::string str, int n)
{
    std::string line;
    std::stringstream s(str);
    for (int i=0; i < n; i++) {
        std::getline(s, line);
    }
    return line;
}

#define T(tk, name) case (yytokentype::tk) : return name;

std::string token2text(const int token)
{
    if (0 < token && token < 256) {
        char t = token;
        return std::string(&t, 1);
    }
    switch (token) {
        T(END_OF_FILE, "EOF")
        T(TK_NAME, "identifier")
        T(TK_DEF_OP, "defined operator")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_NEWLINE, "newline")
        T(TK_STRING, "string")
        T(TK_DBL_DOT, "..")
        T(TK_DBL_COLON, "::")
        T(TK_POW, "**")
        T(TK_CONCAT, "//")
        T(TK_ARROW, "=>")
        T(TK_EQ, "==")
        T(TK_NE, "!=")
        T(TK_LT, "<")
        T(TK_LE, "<=")
        T(TK_GT, ">")
        T(TK_GE, ">=")
        T(TK_NOT, ".not.")
        T(TK_AND, ".and.")
        T(TK_OR, ".or.")
        T(TK_EQV, ".eqv.")
        T(TK_NEQV, ".neqv.")
        T(TK_TRUE, ".true.")
        T(TK_FALSE, ".false.")
        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw std::runtime_error("Token conversion not implemented yet.");
        }
    }
}

void show_syntax_error(const std::string &filename, const std::string &input,
        const Location &loc, const int token)
{
    std::string redon  = "\033[0;31m";
    std::string redoff = "\033[0;00m";
    std::cout << filename << ":" << loc.first_line << ":" << loc.first_column;
    std::cout << " " << redon << "syntax error:" << redoff << " ";
    std::cout << "token '";
    std::cout << token2text(token);
    std::cout << "' is unexpected" << std::endl;
    if (loc.first_line == loc.last_line) {
        std::string line = get_line(input, loc.first_line);
        std::cout << line.substr(0, loc.first_column-1);
        if (loc.last_column <= line.size()) {
            std::cout << redon;
            std::cout << line.substr(loc.first_column-1,
                    loc.last_column-loc.first_column+1);
            std::cout << redoff;
            std::cout << line.substr(loc.last_column);
        }
        std::cout << std::endl;;
        for (int i=0; i < loc.first_column-1; i++) {
            std::cout << " ";
        }
        std::cout << redon << "^";
        for (int i=loc.first_column; i < loc.last_column; i++) {
            std::cout << "~";
        }
        std::cout << redoff << std::endl;
    } else {
        throw std::runtime_error("Multiline errors not implemented yet.");
    }
}

}
