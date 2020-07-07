#include <iostream>
#include <chrono>

#include <bin/CLI11.hpp>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
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
        toks = LFortran::tokens(input, &stypes);
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
    LFortran::Vec<LFortran::AST::ast_t*> ast;
    try {
        ast = LFortran::parsen2(al, input);
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

    for (auto a: ast) {
        std::cout << LFortran::pickle(*a) << std::endl;
    }
    return 0;
}

int emit_asr(const std::string &infile)
{
    std::string input = read_file(infile);

    // Src -> AST
    Allocator al(64*1024*1024);
    LFortran::Vec<LFortran::AST::ast_t*> ast;
    try {
        ast = LFortran::parsen2(al, input);
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
    LFortran::ASR::asr_t* asr;
    try {
        // FIXME: For now we only transform the first node in the list:
        asr = LFortran::ast_to_asr(al, *ast[0]);
    } catch (const LFortran::LFortranException &e) {
        std::cerr << "LFortran exception: " << e.msg() << std::endl;
        return 4;
    }

    std::cout << LFortran::pickle(*asr) << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    std::string arg_file;
    bool arg_version = false;
    bool show_tokens = false;
    bool show_ast = false;
    bool show_asr = false;

    CLI::App app{"cpptranslate: Fortran to C++ translation"};
    app.add_option("file", arg_file, "Source file");
    app.add_flag("--version", arg_version, "Display compiler version information");

    app.add_flag("--show-tokens", show_tokens, "Show tokens for the given file and exit");
    app.add_flag("--show-ast", show_ast, "Show AST for the given file and exit");
    app.add_flag("--show-asr", show_asr, "Show ASR for the given file and exit");
    CLI11_PARSE(app, argc, argv);

    if (arg_version) {
        std::string version = LFORTRAN_VERSION;
        if (version == "0.1.1") version = "git";
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

    return 0;
}
