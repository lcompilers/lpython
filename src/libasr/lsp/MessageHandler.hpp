#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP
 
#include <libasr/asr.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/parser/parser.h>
#include <libasr/pass/pass_manager.h>
#include <lpython/python_ast.h>
 
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
