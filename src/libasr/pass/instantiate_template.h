#ifndef LFORTRAN_PASS_TEMPLATE_VISITOR_H
#define LFORTRAN_PASS_TEMPLATE_VISITOR_H

#include <libasr/asr.h>

namespace LCompilers {

    /**
     * @brief Instantiate a generic function into a function that does not
     *        contain any type parameters and restrictions. No type checking
     *        is executed here
     */
    ASR::symbol_t* pass_instantiate_generic_function(Allocator &al,
        std::map<std::string, ASR::ttype_t*> subs, std::map<std::string, ASR::symbol_t*> rt_subs,
        SymbolTable *current_scope, std::string new_func_name, ASR::symbol_t *sym);
} // namespace LCompilers

#endif // LFORTRAN_PASS_TEMPLATE_VISITOR_H
