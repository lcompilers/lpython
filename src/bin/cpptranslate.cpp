#include <iostream>
#include <chrono>

#define CLI11_HAS_FILESYSTEM 0
#include <bin/CLI11.hpp>

#include <lfortran/parser/parser.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/ast_to_openmp.h>
#include <libasr/config.h>

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

int emit_ast_openmp(const std::string &infile)
{
    std::string input = read_file(infile);

    // Src -> AST
    LFortran::diag::Diagnostics diagnostics;
    Allocator al(64*1024*1024);
    LFortran::AST::TranslationUnit_t* ast;
    ast = LFortran::TRY(LFortran::parse(al, input, diagnostics));

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

    if (show_ast_openmp) {
        return emit_ast_openmp(arg_file);
    }

    return 0;
}
