#ifndef LFORTRAN_PASS_TEMPLATE_VISITOR_H
#define LFORTRAN_PASS_TEMPLATE_VISITOR_H

#include <libasr/asr.h>

namespace LFortran {

    ASR::symbol_t* pass_instantiate_generic_function(Allocator &al, std::map<std::string, ASR::ttype_t*> subs,
        SymbolTable *current_scope, int new_function_num, /* ASR::TemplateFunction_t */ ASR::Function_t &func);
}

#endif // LFORTRAN_PASS_TEMPLATE_VISITOR_H