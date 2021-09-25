#include <iostream>
#include <chrono>

#define CLI11_HAS_FILESYSTEM 0
#include <bin/CLI11.hpp>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/ast_to_openmp.h>
#include <lfortran/config.h>


std::string remove_extension(const std::string& filename) {
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos) return filename;
    return filename.substr(0, lastdot);
}

std::string read_file(const std::string &filename)
{
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary
            | std::ios::ate);

    std::ifstream::pos_type filesize = ifs.tellg();
    if (filesize < 0) return std::string();

    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(filesize);
    ifs.read(&bytes[0], filesize);

    return std::string(&bytes[0], filesize);
}

int emit_tokens(const std::string &infile)
{
    std::string input = read_file(infile);
    // Src -> Tokens
    Allocator al(64*1024*1024);
    std::vector<int> toks;
    std::vector<LFortran::YYSTYPE> stypes;
    try {
        toks = LFortran::tokens(al, input, &stypes);
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << "Tokenizing error: " << e.msg() << std::endl;
        return 1;
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "Other LFortran exception: " << e.msg() << std::endl;
        return 3;
    }

    for (size_t i=0; i < toks.size(); i++) {
        std::cout << LFortran::pickle(toks[i], stypes[i]) << std::endl;
    }
    return 0;
}

int emit_ast(const std::string &infile)
{
    std::string input = read_file(infile);
    // Src -> AST
    Allocator al(64*1024*1024);
    LFortran::AST::TranslationUnit_t* ast;
    try {
        ast = LFortran::parse2(al, input);
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << "Tokenizing error: " << e.msg() << std::endl;
        return 1;
    } catch (const LFortran::ParserError &e) {
        std::cerr << "Parsing error: " << e.msg() << std::endl;
        return 2;
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "Other LFortran exception: " << e.msg() << std::endl;
        return 3;
    }

    std::cout << LFortran::pickle(*ast) << std::endl;
    return 0;
}

int emit_asr(const std::string &infile)
{
    std::string input = read_file(infile);

    // Src -> AST
    Allocator al(64*1024*1024);
    LFortran::AST::TranslationUnit_t* ast;
    try {
        ast = LFortran::parse2(al, input);
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << "Tokenizing error: " << e.msg() << std::endl;
        return 1;
    } catch (const LFortran::ParserError &e) {
        std::cerr << "Parsing error: " << e.msg() << std::endl;
        return 2;
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "Other LFortran exception: " << e.msg() << std::endl;
        return 3;
    }

    // AST -> ASR
    LFortran::ASR::TranslationUnit_t* asr;
    try {
        // FIXME: For now we only transform the first node in the list:
        asr = LFortran::ast_to_asr(al, *ast);
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "LFortran exception: " << e.msg() << std::endl;
        return 4;
    }

    std::cout << LFortran::pickle(*asr) << std::endl;
    return 0;
}

int emit_ast_f90(const std::string &infile)
{
    std::string input = read_file(infile);

    // Src -> AST
    Allocator al(64*1024*1024);
    LFortran::AST::TranslationUnit_t* ast;
    try {
        ast = LFortran::parse2(al, input);
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << "Tokenizing error: " << e.msg() << std::endl;
        return 1;
    } catch (const LFortran::ParserError &e) {
        std::cerr << "Parsing error: " << e.msg() << std::endl;
        return 2;
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "Other LFortran exception: " << e.msg() << std::endl;
        return 3;
    }

    // AST -> Source
    std::string source = LFortran::ast_to_src(*ast);

    std::cout << source << std::endl;
    return 0;
}

int emit_ast_openmp(const std::string &infile)
{
    std::string input = read_file(infile);

    // Src -> AST
    Allocator al(64*1024*1024);
    LFortran::AST::TranslationUnit_t* ast;
    try {
        ast = LFortran::parse2(al, input);
    } catch (const LFortran::TokenizerError &e) {
        std::cerr << "Tokenizing error: " << e.msg() << std::endl;
        return 1;
    } catch (const LFortran::ParserError &e) {
        std::cerr << "Parsing error: " << e.msg() << std::endl;
        return 2;
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "Other LFortran exception: " << e.msg() << std::endl;
        return 3;
    }

    // AST -> Source
    // FIXME: For now we only transform the first node in the list:
    std::string source = LFortran::ast_to_openmp(*ast->m_items[0]);

    std::cout << source << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    std::string arg_file;
    bool arg_version = false;
    bool show_tokens = false;
    bool show_ast = false;
    bool show_asr = false;
    bool show_ast_f90 = false;
    bool show_ast_openmp = false;

    CLI::App app{"cpptranslate: Fortran to C++ translation"};
    app.add_option("file", arg_file, "Source file");
    app.add_flag("--version", arg_version, "Display compiler version information");

    app.add_flag("--show-tokens", show_tokens, "Show tokens for the given file and exit");
    app.add_flag("--show-ast", show_ast, "Show AST for the given file and exit");
    app.add_flag("--show-asr", show_asr, "Show ASR for the given file and exit");
    app.add_flag("--show-ast-f90", show_ast_f90, "Show AST -> Fortran for the given file and exit");
    app.add_flag("--show-ast-openmp", show_ast_openmp, "Show AST -> Fortran with OpenMP for the given file and exit");
    CLI11_PARSE(app, argc, argv);

    if (arg_version) {
        std::string version = LFORTRAN_VERSION;
        std::cout << "LFortran version: " << version << std::endl;
        return 0;
    }

    if (arg_file.size() == 0) {
        std::cerr << "Run 'cpptranslate -h' for help" << std::endl;
        return 1;
    }

    if (show_tokens) {
        return emit_tokens(arg_file);
    }
    if (show_ast) {
        return emit_ast(arg_file);
    }
    if (show_asr) {
        return emit_asr(arg_file);
    }
    if (show_ast_f90) {
        return emit_ast_f90(arg_file);
    }
    if (show_ast_openmp) {
        return emit_ast_openmp(arg_file);
    }

    return 0;
}
