#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

#include <map>

#include <lfortran/parser/alloc.h>

namespace LFortran  {

namespace ASR {
    struct symbol_t;
}

struct SymbolTable {
    std::map<std::string, ASR::symbol_t*> scope;
    SymbolTable *parent;
    unsigned int counter;
    std::vector<std::string> data_member_names;

    SymbolTable(SymbolTable *parent);

    // Determines a stable hash based on the content of the symbol table
    uint32_t get_hash_uint32(); // Returns the hash as an integer
    std::string get_hash();     // Returns the hash as a hex string
    std::string get_counter() {  // Returns a unique ID as a string
        return std::to_string(counter);
    }

    // Resolves the symbol `name` recursively in current and parent scopes.
    // Returns `nullptr` if symbol not found.
    ASR::symbol_t* resolve_symbol(const std::string &name) {
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
