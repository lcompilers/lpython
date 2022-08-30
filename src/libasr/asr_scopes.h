#ifndef LFORTRAN_SEMANTICS_ASR_SCOPES_H
#define LFORTRAN_SEMANTICS_ASR_SCOPES_H

#include <map>

#include <libasr/alloc.h>

namespace LFortran  {

namespace ASR {
    struct asr_t;
    struct symbol_t;
}

struct SymbolTable {
    private:
    std::map<std::string, ASR::symbol_t*> scope;

    public:
    SymbolTable *parent;
    // The ASR node (either symbol_t or TranslationUnit_t) that contains this
    // SymbolTable as m_symtab / m_global_scope member. One of:
    // * symbol_symtab(down_cast<symbol_t>(this->asr_owner)) == this
    // * down_cast2<TranslationUnit_t>(this->asr_owner)->m_global_scope == this
    ASR::asr_t *asr_owner = nullptr;
    unsigned int counter;

    SymbolTable(SymbolTable *parent);

    // Determines a stable hash based on the content of the symbol table
    uint32_t get_hash_uint32(); // Returns the hash as an integer
    std::string get_counter() {  // Returns a unique ID as a string
        return std::to_string(counter);
    }
    static void reset_global_counter(); // Resets the internal global counter

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

    const std::map<std::string, ASR::symbol_t*>& get_scope() const {
        return scope;
    }

    // Obtains the symbol `name` from the current symbol table
    // Returns `nullptr` if symbol not found.
    ASR::symbol_t* get_symbol(const std::string &name) const {
        //auto it = scope.find(to_lower(name));
        auto it = scope.find(name);
        if (it == scope.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }

    void erase_symbol(const std::string &name) {
        //auto it = scope.find(to_lower(name));
        scope.erase(name);
    }

    void add_symbol(const std::string &name, ASR::symbol_t* symbol) {
        scope[name] = symbol;
    }

    // Marks all variables as external
    void mark_all_variables_external(Allocator &al);

    ASR::symbol_t *find_scoped_symbol(const std::string &name,
        size_t n_scope_names, char **m_scope_names);

    std::string get_unique_name(const std::string &name);
};

} // namespace LFortran

#endif // LFORTRAN_SEMANTICS_ASR_SCOPES_H
