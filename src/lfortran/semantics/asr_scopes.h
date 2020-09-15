#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

#include <map>

#include <lfortran/parser/alloc.h>

namespace LFortran  {

namespace ASR {
    struct asr_t;
}

struct SymbolTable {
    std::map<std::string, ASR::asr_t*> scope;
    SymbolTable *parent;

    SymbolTable(SymbolTable *parent) : parent{parent} {}

    // Determines a stable hash based on the content of the symbol table
    uint32_t get_hash_uint32(); // Returns the hash as an integer
    std::string get_hash();     // Returns the hash as a hex string

    // Resolves the symbol `name` recursively in current and parent scopes.
    // Returns `nullptr` if symbol not found.
    ASR::asr_t* resolve_symbol(const std::string &name) {
        if (scope.find(name) == scope.end()) {
            if (parent) {
                return parent->resolve_symbol(name);
            } else {
                return nullptr;
            }
        }
        return scope[name];
    }

    // Marks all variables as external
    void mark_all_variables_external(Allocator &al);
};

} // namespace LFortran

#endif // LFORTRAN_SEMANTICS_ASR_SCOPES_H
