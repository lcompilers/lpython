#ifndef LIBASR_PASS_INSTANTIATE_TEMPLATE_H
#define LIBASR_PASS_INSTANTIATE_TEMPLATE_H

#include <libasr/asr.h>
#include <libasr/utils.h>

namespace LCompilers {

    ASR::symbol_t* pass_instantiate_template(Allocator &al,
        std::map<std::string, ASR::ttype_t*> subs, std::map<std::string, ASR::symbol_t*> rt_subs,
        SymbolTable *current_scope, std::string new_func_name, ASR::symbol_t *sym);

} // namespace LCompilers

#endif // LIBASR_PASS_INSTANTIATE_TEMPLATE_H
