#ifndef LPYTHON_PARSER_PARSER_H
#define LPYTHON_PARSER_PARSER_H

#include <libasr/containers.h>
#include <libasr/diagnostics.h>
#include <lpython/parser/tokenizer.h>

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
    Vec<LPython::AST::stmt_t*> result;
    Vec<LPython::AST::type_ignore_t*> type_ignore;

    Parser(Allocator &al, diag::Diagnostics &diagnostics)
            : diag{diagnostics}, m_a{al} {
        result.reserve(al, 32);
        type_ignore.reserve(al, 4);
    }

    void parse(const std::string &input);
    void handle_yyerror(const Location &loc, const std::string &msg);
};


// Parses Python code to AST
Result<LPython::AST::Module_t*> parse(Allocator &al,
    const std::string &s,
    diag::Diagnostics &diagnostics);

Result<LPython::AST::ast_t*> parse_python_file(Allocator &al,
        const std::string &runtime_library_dir,
        const std::string &infile,
        diag::Diagnostics &diagnostics,
        bool new_parser);

} // namespace LFortran

#endif
