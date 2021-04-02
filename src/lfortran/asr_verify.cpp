#include <lfortran/containers.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>


namespace LFortran {
namespace ASR {

class VerifyVisitor : public BaseWalkVisitor<VerifyVisitor>
{
private:
    // For checking correct parent symbtab relationship
    SymbolTable *current_symtab;

    // For checking that all symtabs have a unique ID.
    // We first walk all symtabs, and then we check that everything else
    // points to them (i.e., that nothing points to some symbol table that
    // is not part of this ASR).
    std::map<uint64_t,SymbolTable*> id_symtab_map;
    bool check_external;
public:
    VerifyVisitor(bool check_external) : check_external{check_external} {}

    // Requires the condition `cond` to be true. Raise an exception otherwise.
    void require(bool cond, const std::string &error_msg) {
        if (!cond) {
            throw LFortranException("ASR verify failed: " + error_msg);
        }
    }

    // Returns true if the `symtab_ID` is the current symbol table `symtab` or
    // any of its parents. It returns false otherwise, such as in the case when
    // the symtab is in a different module.
    bool symtab_in_scope(const SymbolTable *symtab, unsigned int symtab_ID) {
        const SymbolTable *s = symtab;
        while (s != nullptr) {
            if (s->counter == symtab_ID) return true;
            s = s->parent;
        }
        return false;
    }

    void visit_TranslationUnit(const TranslationUnit_t &x) {
        current_symtab = x.m_global_scope;
        require(x.m_global_scope != nullptr,
            "The TranslationUnit::m_global_scope cannot be nullptr");
        require(x.m_global_scope->parent == nullptr,
            "The TranslationUnit::m_global_scope->parent must be nullptr");
        require(id_symtab_map.find(x.m_global_scope->counter) == id_symtab_map.end(),
            "TranslationUnit::m_global_scope->counter must be unique");
        id_symtab_map[x.m_global_scope->counter] = x.m_global_scope;
        for (auto &a : x.m_global_scope->scope) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_items; i++) {
            asr_t *item = x.m_items[i];
            require(is_a<stmt_t>(*item) || is_a<expr_t>(*item),
                "TranslationUnit::m_items must be either stmt or expr");
        }
        current_symtab = nullptr;
    }

    // --------------------------------------------------------
    // symbol instances:

    void visit_Program(const Program_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The Program::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The Program::m_symtab->parent is not the right parent");
        require(x.m_symtab->parent->parent == nullptr,
            "The Program::m_symtab's parent must be TranslationUnit");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Program::m_symtab->counter must be unique");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        current_symtab = parent_symtab;
    }

    void visit_Module(const Module_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The Module::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The Module::m_symtab->parent is not the right parent");
        require(x.m_symtab->parent->parent == nullptr,
            "The Module::m_symtab's parent must be TranslationUnit");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Module::m_symtab->counter must be unique");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Subroutine(const Subroutine_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The Subroutine::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The Subroutine::m_symtab->parent is not the right parent");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Subroutine::m_symtab->counter must be unique");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        current_symtab = parent_symtab;
    }

    void visit_Function(const Function_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The Function::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The Function::m_symtab->parent is not the right parent");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Function::m_symtab->counter must be unique");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        visit_expr(*x.m_return_var);
        current_symtab = parent_symtab;
    }

    void visit_DerivedType(const DerivedType_t &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The DerivedType::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The DerivedType::m_symtab->parent is not the right parent");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Derivedtype::m_symtab->counter must be unique");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->scope) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_Variable(const Variable_t &x) {
        SymbolTable *symtab = x.m_parent_symtab;
        require(symtab != nullptr,
            "Variable::m_parent_symtab cannot be nullptr");
        require(symtab->scope.find(std::string(x.m_name)) != symtab->scope.end(),
            "Variable not found in parent_symtab symbol table");
        symbol_t *symtab_sym = symtab->scope[std::string(x.m_name)];
        const symbol_t *current_sym = &x.base;
        require(symtab_sym == current_sym,
            "Variable's parent symbol table does not point to it");
        require(id_symtab_map.find(symtab->counter) != id_symtab_map.end(),
            "Variable::m_parent_symtab must be present in the ASR");

        if (x.m_value)
            visit_expr(*x.m_value);
        visit_ttype(*x.m_type);
    }

    void visit_ExternalSymbol(const ExternalSymbol_t &x) {
        if (check_external) {
            require(x.m_external != nullptr,
                "ExternalSymbol::m_external cannot be nullptr");
            require(!is_a<ExternalSymbol_t>(*x.m_external),
                "ExternalSymbol::m_external cannot be an ExternalSymbol");
            char *orig_name = symbol_name(x.m_external);
            require(std::string(x.m_original_name) == std::string(orig_name),
                "ExternalSymbol::m_original_name must match external->m_name");
            // TODO: check that module name matches x.m_module_name
        }
    }

    // --------------------------------------------------------
    // nodes that have symbol in their fields:

    void visit_Var(const Var_t &x) {
        require(x.m_v != nullptr,
            "Var_t::m_v cannot be nullptr");
        require(is_a<Variable_t>(*x.m_v) || is_a<ExternalSymbol_t>(*x.m_v),
            "Var_t::m_v does not point to a Variable_t or ExternalSymbol_t");
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_v)->counter),
            "Var::m_v cannot point outside of its symbol table");
    }

    void visit_ArrayRef(const ArrayRef_t &x) {
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_v)->counter),
            "ArrayRef::m_v cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_args; i++) {
            visit_array_index(x.m_args[i]);
        }
        visit_ttype(*x.m_type);
    }

    void visit_SubroutineCall(const SubroutineCall_t &x) {
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_name)->counter),
            "SubroutineCall::m_name cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
    }

    void visit_FunctionCall(const FunctionCall_t &x) {
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_name)->counter),
            "FunctionCall::m_name cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
        for (size_t i=0; i<x.n_keywords; i++) {
            visit_keyword(x.m_keywords[i]);
        }
        visit_ttype(*x.m_type);
    }

    void visit_Derived(const Derived_t &x) {
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_derived_type)->counter),
            "Derived::m_derived_type cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_dims; i++) {
            visit_dimension(x.m_dims[i]);
        }
    }

    void visit_DerivedPointer(const DerivedPointer_t &x) {
        require(symtab_in_scope(current_symtab,
             symbol_parent_symtab(x.m_derived_type)->counter),
            "DerivedPointer::m_derived_type cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_dims; i++) {
            visit_dimension(x.m_dims[i]);
        }
    }

};


} // namespace ASR

bool asr_verify(const ASR::TranslationUnit_t &unit, bool check_external) {
    ASR::VerifyVisitor v(check_external);
    v.visit_TranslationUnit(unit);
    return true;
}

} // namespace LFortran
