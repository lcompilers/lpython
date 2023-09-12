#include <iomanip>
#include <sstream>

#include <libasr/asr_scopes.h>
#include <libasr/asr_utils.h>

std::string lcompilers_unique_ID;

namespace LCompilers  {

template< typename T >
std::string hexify(T i)
{
    std::stringbuf buf;
    std::ostream os(&buf);
    os << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return buf.str();
}

unsigned int symbol_table_counter = 0;

SymbolTable::SymbolTable(SymbolTable *parent) : parent{parent} {
    symbol_table_counter++;
    counter = symbol_table_counter;
}

void SymbolTable::reset_global_counter() {
    symbol_table_counter = 0;
}

void SymbolTable::mark_all_variables_external(Allocator &al) {
    for (auto &a : scope) {
        switch (a.second->type) {
            case (ASR::symbolType::Variable) : {
                ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(a.second);
                v->m_abi = ASR::abiType::Interactive;
                break;
            }
            case (ASR::symbolType::Function) : {
                ASR::Function_t *v = ASR::down_cast<ASR::Function_t>(a.second);
                ASR::FunctionType_t* v_func_type = ASR::down_cast<ASR::FunctionType_t>(v->m_function_signature);
                v_func_type->m_abi = ASR::abiType::Interactive;
                v->m_body = nullptr;
                v->n_body = 0;
                break;
            }
            case (ASR::symbolType::Module) : {
                ASR::Module_t *v = ASR::down_cast<ASR::Module_t>(a.second);
                v->m_symtab->mark_all_variables_external(al);
            }
            default : {};
        }
    }
}

ASR::symbol_t *SymbolTable::find_scoped_symbol(const std::string &name,
        size_t n_scope_names, char **m_scope_names) {
    const SymbolTable *s = this;
    for(size_t i=0; i < n_scope_names; i++) {
        std::string scope_name = m_scope_names[i];
        if (s->scope.find(scope_name) != scope.end()) {
            ASR::symbol_t *sym = s->scope.at(scope_name);
            s = ASRUtils::symbol_symtab(sym);
            if (s == nullptr) {
                // The m_scope_names[i] found in the appropriate symbol table,
                // but points to a symbol that itself does not have a symbol
                // table
                return nullptr;
            }
        } else {
            // The m_scope_names[i] not found in the appropriate symbol table
            return nullptr;
        }
    }
    if (s->scope.find(name) != scope.end()) {
        ASR::symbol_t *sym = s->scope.at(name);
        LCOMPILERS_ASSERT(sym)
        return sym;
    } else {
        // The `name` not found in the appropriate symbol table
        return nullptr;
    }
}

std::string SymbolTable::get_unique_name(const std::string &name, bool use_unique_id) {
    std::string unique_name = name;
    if( use_unique_id && !lcompilers_unique_ID.empty()) {
        unique_name += "_" + lcompilers_unique_ID;
    }
    int counter = 1;
    while (scope.find(unique_name) != scope.end()) {
        unique_name = name + std::to_string(counter);
        counter++;
    }
    return unique_name;
}

} // namespace LCompilers
