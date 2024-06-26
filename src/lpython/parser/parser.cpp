#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>

#include <lpython/parser/parser.h>
#include <lpython/parser/parser.tab.hh>
#include <libasr/diagnostics.h>
#include <libasr/string_utils.h>
#include <libasr/utils.h>
#include <lpython/parser/parser_exception.h>
#include <lpython/python_serialization.h>

namespace LCompilers::LPython {

Result<LPython::AST::Module_t*> parse(Allocator &al, const std::string &s,
        uint32_t prev_loc, diag::Diagnostics &diagnostics)
{
    Parser p(al, diagnostics);
    try {
        p.parse(s, prev_loc);
    } catch (const parser_local::TokenizerError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const parser_local::ParserError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    }

    Location l;
    if (p.result.size() == 0) {
        l.first=0;
        l.last=0;
    } else {
        l.first=p.result[0]->base.loc.first;
        l.last=p.result[p.result.size()-1]->base.loc.last;
    }
    return (LPython::AST::Module_t*)LPython::AST::make_Module_t(al, l,
        p.result.p, p.result.size(), p.type_ignore.p, p.type_ignore.size());
}

void Parser::parse(const std::string &input, uint32_t prev_loc)
{
    inp = input;
    if (inp.size() > 0) {
        if (inp[inp.size()-1] != '\n') inp.append("\n");
    } else {
        inp.append("\n");
    }
    m_tokenizer.set_string(inp, prev_loc);
    if (yyparse(*this) == 0) {
        return;
    }
    throw parser_local::ParserError("Parsing unsuccessful (internal compiler error)");
}

void Parser::handle_yyerror(const Location &loc, const std::string &msg)
{
    std::string message;
    if (msg == "syntax is ambiguous") {
        message = "Internal Compiler Error: syntax is ambiguous in the parser";
    } else if (msg == "syntax error") {
        YYSTYPE yylval_;
        YYLTYPE yyloc_;
        this->m_tokenizer.cur = this->m_tokenizer.tok;
        int token = this->m_tokenizer.lex(this->m_a, yylval_, yyloc_, diag);
        if (token == yytokentype::END_OF_FILE) {
            message =  "End of file is unexpected here";
        } else if (token == yytokentype::TK_NEWLINE) {
            message =  "Newline is unexpected here";
        } else {
            std::string token_str = this->m_tokenizer.token();
            std::string token_type = token2text(token);
            if (token_str == token_type) {
                message =  "Token '" + token_str + "' is unexpected here";
            } else {
                message =  "Token '" + token_str + "' (of type '" + token2text(token) + "') is unexpected here";
            }
        }
    } else {
        message = "Internal Compiler Error: parser returned unknown error";
    }
    throw parser_local::ParserError(message, loc);
}

bool file_exists(const std::string &name) {
    std::ifstream file(name);
    if (!file.is_open()) {
        return false;
    }
    return true;
}

std::string unique_filename(const std::string &prefix) {
    uint64_t ms = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    ms = ms % 1000000000;
    srand((unsigned) ms);
    std::string hex = "0123456789ABCDEF";
    std::string random_hash;
    for (int i=0; i < 6; i++) {
        random_hash += hex[rand() % 16];
    }
    int counter = 1;
    std::string filename = prefix + random_hash + std::to_string(counter);
    while (file_exists(filename)) {
        counter++;
        filename = prefix + random_hash + std::to_string(counter);
    }
    return filename;
}

Result<LPython::AST::ast_t*> parse_python_source(Allocator &al,
        const std::string &/*runtime_library_dir*/,
        const std::string &source_code,
        diag::Diagnostics &diagnostics,
        uint32_t prev_loc,
        [[maybe_unused]] bool new_parser) {
    LPython::AST::ast_t* ast;
    // We will be using the new parser from now on
    new_parser = true;
    LCOMPILERS_ASSERT(new_parser)
    Result<LPython::AST::Module_t*> res = parse(al, source_code, prev_loc, diagnostics);
    if (res.ok) {
        ast = (LPython::AST::ast_t*)res.result;
    } else {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return Error();
    }
    return ast;
}

Result<LPython::AST::ast_t*> parse_python_file(Allocator &al,
        const std::string &runtime_library_dir,
        const std::string &infile,
        diag::Diagnostics &diagnostics,
        uint32_t prev_loc,
        [[maybe_unused]] bool new_parser) {
    std::string input_source_code = read_file(infile);
    return parse_python_source(al, runtime_library_dir, input_source_code, diagnostics, prev_loc, new_parser);
}


} // namespace LCompilers::LPython
