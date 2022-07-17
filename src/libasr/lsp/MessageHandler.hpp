#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <lpython/utils.h>
#include <lpython/parser/parser.h>
#include <libasr/string_utils.h>
#include <lpython/semantics/python_ast_to_asr.h>
 
 namespace LFortran::LPython {
       struct lsp_locations {
              std::string symbol_name;
              uint32_t first_line;
              uint32_t first_column;
              uint32_t last_line;
              uint32_t last_column;
       };
       std::vector<lsp_locations> get_SymbolLists(const std::string &infile,
       const std::string &runtime_library_dir,
       LFortran::CompilerOptions &compiler_options);
}
#endif
