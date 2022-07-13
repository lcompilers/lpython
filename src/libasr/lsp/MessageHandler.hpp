
#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP
 
#include <libasr/asr.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/parser/parser.h>
#include <libasr/pass/pass_manager.h>
#include <lpython/python_ast.h>
 
namespace LFortran::LPython {
       std::vector<lsp_locations> get_SymbolLists(const std::string &infile,
       LCompilers::PassManager& pass_manager,
       const std::string &runtime_library_dir,
       LFortran::CompilerOptions &compiler_options);
}
#endif
