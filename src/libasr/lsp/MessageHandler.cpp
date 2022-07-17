#include <iostream>
#include "MessageHandler.hpp"

namespace LFortran::LPython {
    std::vector<lsp_locations> get_SymbolLists(const std::string &infile,
        const std::string &runtime_library_dir,
        LFortran::CompilerOptions &compiler_options) {
            Allocator al(4*1024);
            LFortran::diag::Diagnostics diagnostics;
            LFortran::LocationManager lm;
            lm.in_filename = infile;
            std::string input = LFortran::read_file(infile);
            lm.init_simple(input);
            LFortran::Result<LFortran::LPython::AST::ast_t*> r1 = LFortran::parse_python_file(
                al, runtime_library_dir, infile, diagnostics, compiler_options.new_parser);
            LFortran::LPython::AST::ast_t* ast = r1.result;
            LFortran::Result<LFortran::ASR::TranslationUnit_t*> x = LFortran::LPython::python_ast_to_asr(al, *ast, diagnostics, true,
            compiler_options.disable_main, compiler_options.symtab_only, infile);
            std::vector<lsp_locations> symbol_lists;
            lsp_locations loc;
            for (auto &a : x.result->m_global_scope->get_scope()) {
                std::string symbol_name = a.first;
                uint32_t first_line;
                uint32_t last_line;
                uint32_t first_column;
                uint32_t last_column;
                lm.pos_to_linecol(a.second->base.loc.first, first_line, first_column);
                lm.pos_to_linecol(a.second->base.loc.last, last_line, last_column);
                loc.first_column = first_column;
                loc.last_column = last_column;
                loc.first_line = first_line;
                loc.last_line = last_line;
                loc.symbol_name = symbol_name;
                symbol_lists.push_back(loc);
            }
            return symbol_lists;
       }
}

