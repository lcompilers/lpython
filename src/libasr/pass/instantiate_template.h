#ifndef LIBASR_PASS_INSTANTIATE_TEMPLATE_H
#define LIBASR_PASS_INSTANTIATE_TEMPLATE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    /**
     * @brief Instantiate a generic function into a function that does not
     *        contain any type parameters and restrictions. No type checking
     *        is executed here
     */
    ASR::symbol_t* pass_instantiate_symbol(Allocator &al,
        std::map<std::string, std::string> context_map,
        std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> symbol_subs,
        SymbolTable *current_scope, SymbolTable *template_scope,
       std::string new_sym_name, ASR::symbol_t *sym);

    ASR::symbol_t* pass_instantiate_function_body(Allocator &al,
        std::map<std::string, std::string> context_map,
        std::map<std::string, ASR::ttype_t*> type_subs,
        std::map<std::string, ASR::symbol_t*> symbol_subs,
        SymbolTable *current_scope, SymbolTable *template_scope,
        ASR::Function_t *new_f, ASR::Function_t *f);

} // namespace LCompilers

#endif // LIBASR_PASS_INSTANTIATE_TEMPLATE_H
