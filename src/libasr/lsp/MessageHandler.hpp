#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <lpython/utils.h>
#include <lpython/parser/parser.h>
#include <libasr/string_utils.h>
#include <libasr/diagnostics.h>
#include <lpython/semantics/python_ast_to_asr.h>
 
 namespace LFortran::LPython {
       
       struct lsp_locations {
              std::string symbol_name;
              uint32_t first_line;
              uint32_t first_column;
              uint32_t last_line;
              uint32_t last_column;
       };
       struct lsp_highlight {
              std::string message;
              uint32_t first_line;
              uint32_t first_column;
              uint32_t last_line;
              uint32_t last_column;
              uint32_t severity;
       };
       std::vector<lsp_locations> get_SymbolLists(const std::string &infile,
              const std::string &runtime_library_dir,
              LFortran::CompilerOptions &compiler_options);
       std::vector<lsp_highlight> get_Diagnostics(const std::string &infile,
              const std::string &get_runtime_library_dir,
              LFortran::CompilerOptions &compiler_options);
       
}
#endif
