#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>


namespace LFortran {
namespace ASR {

using LFortran::ASRUtils::symbol_name;
using LFortran::ASRUtils::symbol_parent_symtab;

bool valid_char(char c) {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '_') return true;
    return false;
}

bool valid_name(const char *s) {
    if (s == nullptr) return false;
    std::string name = s;
    if (name.size() == 0) return false;
    for (size_t i=0; i<name.size(); i++) {
        if (!valid_char(s[i])) return false;
    }
    return true;
}

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
            throw LCompilersException("ASR verify failed: " + error_msg);
        }
    }
    void require(bool cond, const std::string &error_msg,
                const Location &loc) {
        std::string msg = std::to_string(loc.first) + ":"
            + std::to_string(loc.last) + ": " + error_msg;
        /*
        std::string msg = std::to_string(loc.first_line) + ":"
            + std::to_string(loc.first_column) + ": " + error_msg;
        */
        require(cond, msg);
    }

    // Returns true if the `symtab_ID` (sym->symtab->parent) is the current
    // symbol table `symtab` or any of its parents *and* if the symbol in the
    // symbol table is equal to `sym`. It returns false otherwise, such as in the
    // case when the symtab is in a different module or if the `sym`'s symbol table
    // does not actually contain it.
    bool symtab_in_scope(const SymbolTable *symtab, const ASR::symbol_t *sym) {
        unsigned int symtab_ID = symbol_parent_symtab(sym)->counter;
        char *sym_name = symbol_name(sym);
        const SymbolTable *s = symtab;
        while (s != nullptr) {
            if (s->counter == symtab_ID) {
                ASR::symbol_t *sym2 = s->get_symbol(sym_name);
                if (sym2) {
                    if (sym2 == sym) {
                        // The symbol table was found and the symbol `sym` is in
                        // it
                        return true;
                    } else {
                        // The symbol table was found and the symbol in it
                        // shares the name, but is not equal to `sym`
                        return false;
                    }
                } else {
                    // The symbol table was found, but the symbol `sym` is not
                    // in it
                    return false;
                }
            }
            s = s->parent;
        }
        // The symbol table was not found in the scope of `symtab`.
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
        require(x.m_global_scope->asr_owner == (ASR::asr_t*)&x,
            "The TranslationUnit::m_global_scope::asr_owner must point to itself");
        require(down_cast2<TranslationUnit_t>(current_symtab->asr_owner)->m_global_scope == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_global_scope->counter] = x.m_global_scope;
        for (auto &a : x.m_global_scope->get_scope()) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_items; i++) {
            asr_t *item = x.m_items[i];
            require(is_a<stmt_t>(*item) || is_a<expr_t>(*item),
                "TranslationUnit::m_items must be either stmt or expr");
            if (is_a<stmt_t>(*item)) {
                this->visit_stmt(*down_cast<stmt_t>(item));
            } else {
                this->visit_expr(*down_cast<expr_t>(item));
            }
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
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        current_symtab = parent_symtab;
    }

    void visit_AssociateBlock(const AssociateBlock_t& x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The AssociateBlock::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The AssociateBlock::m_symtab->parent is not the right parent");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "AssociateBlock::m_symtab->counter must be unique");
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        current_symtab = parent_symtab;
    }

    void visit_Block(const Block_t& x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The AssociateBlock::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The AssociateBlock::m_symtab->parent is not the right parent");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "AssociateBlock::m_symtab->counter must be unique");
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
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
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i < x.n_dependencies; i++) {
            require(x.m_dependencies[i] != nullptr,
                "A module dependency must not be a nullptr",
                x.base.base.loc);
            require(std::string(x.m_dependencies[i]) != "",
                "A module dependency must not be an empty string",
                x.base.base.loc);
            require(valid_name(x.m_dependencies[i]),
                "A module dependency must be a valid string",
                x.base.base.loc);
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
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "Function::m_symtab->counter must be unique");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
        }
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        if (x.m_return_var) {
            visit_expr(*x.m_return_var);
        }
        current_symtab = parent_symtab;
    }

    template <typename T>
    void visit_StructTypeEnumTypeUnionType(const T &x) {
        SymbolTable *parent_symtab = current_symtab;
        current_symtab = x.m_symtab;
        require(x.m_symtab != nullptr,
            "The StructType::m_symtab cannot be nullptr");
        require(x.m_symtab->parent == parent_symtab,
            "The StructType::m_symtab->parent is not the right parent");
        require(x.m_symtab->asr_owner == (ASR::asr_t*)&x,
            "The X::m_symtab::asr_owner must point to X");
        require(id_symtab_map.find(x.m_symtab->counter) == id_symtab_map.end(),
            "StructType::m_symtab->counter must be unique");
        require(ASRUtils::symbol_symtab(down_cast<symbol_t>(current_symtab->asr_owner)) == current_symtab,
            "The asr_owner invariant failed");
        id_symtab_map[x.m_symtab->counter] = x.m_symtab;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
        }
        current_symtab = parent_symtab;
    }

    void visit_StructType(const StructType_t& x) {
        visit_StructTypeEnumTypeUnionType(x);
    }

    void visit_EnumType(const EnumType_t& x) {
        visit_StructTypeEnumTypeUnionType(x);
        require(x.m_type != nullptr,
            "The common type of Enum cannot be nullptr. " +
            std::string(x.m_name) + " doesn't seem to follow this rule.");
        ASR::ttype_t* common_type = x.m_type;
        std::map<int64_t, int64_t> value2count;
        for( auto itr: x.m_symtab->get_scope() ) {
            ASR::Variable_t* itr_var = ASR::down_cast<ASR::Variable_t>(itr.second);
            require(itr_var->m_symbolic_value != nullptr,
                "All members of Enum must have their values to be set. " +
                std::string(itr_var->m_name) + " doesn't seem to follow this rule in "
                + std::string(x.m_name) + " Enum.");
            require(ASRUtils::check_equal_type(itr_var->m_type, common_type),
                "All members of Enum must the same type. " +
                std::string(itr_var->m_name) + " doesn't seem to follow this rule in " +
                std::string(x.m_name) + " Enum.");
            ASR::expr_t* value = ASRUtils::expr_value(itr_var->m_symbolic_value);
            int64_t value_int64 = -1;
            ASRUtils::extract_value(value, value_int64);
            if( value2count.find(value_int64) == value2count.end() ) {
                value2count[value_int64] = 0;
            }
            value2count[value_int64] += 1;
        }

        bool is_enumtype_correct = false;
        bool is_enum_integer = ASR::is_a<ASR::Integer_t>(*x.m_type);
        if( x.m_enum_value_type == ASR::enumtypeType::IntegerConsecutiveFromZero ) {
            is_enumtype_correct = (is_enum_integer &&
                                   (value2count.find(0) != value2count.end()) &&
                                   (value2count.size() == x.n_members));
            int64_t prev = -1;
            if( is_enumtype_correct ) {
                for( auto enum_value: value2count ) {
                    if( enum_value.first - prev != 1 ) {
                        is_enumtype_correct = false;
                        break ;
                    }
                    prev = enum_value.first;
                }
            }
        } else if( x.m_enum_value_type == ASR::enumtypeType::IntegerNotUnique ) {
            is_enumtype_correct = is_enum_integer && (value2count.size() != x.n_members);
        } else if( x.m_enum_value_type == ASR::enumtypeType::IntegerUnique ) {
            is_enumtype_correct = is_enum_integer && (value2count.size() == x.n_members);
        } else if( x.m_enum_value_type == ASR::enumtypeType::NonInteger ) {
            is_enumtype_correct = !is_enum_integer;
        }
        require(is_enumtype_correct, "Properties of enum value members don't match correspond "
                                     "to EnumType::m_enum_value_type");
    }

    void visit_UnionType(const UnionType_t& x) {
        visit_StructTypeEnumTypeUnionType(x);
    }

    void visit_Variable(const Variable_t &x) {
        SymbolTable *symtab = x.m_parent_symtab;
        require(symtab != nullptr,
            "Variable::m_parent_symtab cannot be nullptr");
        require(symtab->get_symbol(std::string(x.m_name)) != nullptr,
            "Variable '" + std::string(x.m_name) + "' not found in parent_symtab symbol table");
        symbol_t *symtab_sym = symtab->get_symbol(std::string(x.m_name));
        const symbol_t *current_sym = &x.base;
        require(symtab_sym == current_sym,
            "Variable's parent symbol table does not point to it");
        require(id_symtab_map.find(symtab->counter) != id_symtab_map.end(),
            "Variable::m_parent_symtab must be present in the ASR ("
                + std::string(x.m_name) + ")");

        if (x.m_symbolic_value)
            visit_expr(*x.m_symbolic_value);
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
            ASR::Module_t *m = ASRUtils::get_sym_module(x.m_external);
            require(m,
                "ExternalSymbol::m_external is not in a module");
            require(std::string(x.m_module_name) == std::string(m->m_name),
                "ExternalSymbol::m_module_name `" + std::string(x.m_module_name)
                + "` must match external's module name `" + std::string(m->m_name) + "`");
            ASR::symbol_t *s = m->m_symtab->find_scoped_symbol(x.m_original_name, x.n_scope_names, x.m_scope_names);
            require(s != nullptr,
                "ExternalSymbol::m_original_name ('"
                + std::string(x.m_original_name)
                + "') + scope_names not found in a module '"
                + std::string(m->m_name) + "'");
            require(s == x.m_external,
                "ExternalSymbol::m_name + scope_names found but not equal to m_external");
        }
    }

    // --------------------------------------------------------
    // nodes that have symbol in their fields:

    void visit_Var(const Var_t &x) {
        require(x.m_v != nullptr,
            "Var_t::m_v cannot be nullptr");
        require(is_a<Variable_t>(*x.m_v) || is_a<ExternalSymbol_t>(*x.m_v)
                || is_a<Function_t>(*x.m_v) || is_a<ASR::EnumType_t>(*x.m_v),
            "Var_t::m_v " + std::string(ASRUtils::symbol_name(x.m_v)) + " does not point to a Variable_t, ExternalSymbol_t, " \
            "Function_t, Subroutine_t or EnumType_t");
        require(symtab_in_scope(current_symtab, x.m_v),
            "Var::m_v `" + std::string(ASRUtils::symbol_name(x.m_v)) + "` cannot point outside of its symbol table");
    }

    template <typename T>
    void visit_ArrayItemSection(const T &x) {
        visit_expr(*x.m_v);
        for (size_t i=0; i<x.n_args; i++) {
            visit_array_index(x.m_args[i]);
        }
        visit_ttype(*x.m_type);
    }

    void visit_ArrayItem(const ArrayItem_t &x) {
        visit_ArrayItemSection(x);
    }

    void visit_ArraySection(const ArraySection_t &x) {
        visit_ArrayItemSection(x);
    }

    void visit_SubroutineCall(const SubroutineCall_t &x) {
        if (x.m_dt) {
            SymbolTable *symtab = get_dt_symtab(x.m_dt, x.base.base.loc);
            bool result = symtab_in_scope(symtab, x.m_name);
            ASR::symbol_t* parent = get_parent_type_dt(x.m_dt, x.base.base.loc);
            while( !result && parent ) {
                symtab = get_dt_symtab(parent, x.base.base.loc);
                result = symtab_in_scope(symtab, x.m_name);
                parent = get_parent_type_dt(parent, x.base.base.loc);
            }
            require(symtab_in_scope(symtab, x.m_name),
                "SubroutineCall::m_name cannot point outside of its symbol table",
                x.base.base.loc);
        } else {
            require(symtab_in_scope(current_symtab, x.m_name),
                "SubroutineCall::m_name '" + std::string(symbol_name(x.m_name)) + "' cannot point outside of its symbol table",
                x.base.base.loc);
        }
        for (size_t i=0; i<x.n_args; i++) {
            if( x.m_args[i].m_value ) {
                visit_expr(*(x.m_args[i].m_value));
            }
        }
    }

    SymbolTable *get_dt_symtab(ASR::symbol_t *dt, const Location &loc) {
        LFORTRAN_ASSERT(dt)
        SymbolTable *symtab = ASRUtils::symbol_symtab(ASRUtils::symbol_get_past_external(dt));
        require(symtab,
            "m_dt::m_v::m_type::class/derived_type must point to a symbol with a symbol table",
            loc);
        return symtab;
    }

    SymbolTable *get_dt_symtab(ASR::expr_t *dt, const Location &loc) {
        require(ASR::is_a<ASR::Var_t>(*dt),
            "m_dt must point to a Var",
            loc);
        ASR::Var_t *var = ASR::down_cast<ASR::Var_t>(dt);
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var->m_v);
        ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(v->m_type);
        ASR::symbol_t *type_sym=nullptr;
        switch (t2->type) {
            case (ASR::ttypeType::Struct): {
                type_sym = ASR::down_cast<ASR::Struct_t>(t2)->m_derived_type;
                break;
            }
            case (ASR::ttypeType::Class): {
                type_sym = ASR::down_cast<ASR::Class_t>(t2)->m_class_type;
                break;
            }
            default :
                require(false,
                    "m_dt::m_v::m_type must point to a type with a symbol table (Struct or Class)",
                    loc);
        }
        return get_dt_symtab(type_sym, loc);
    }

    ASR::symbol_t *get_parent_type_dt(ASR::symbol_t *dt, const Location &loc) {
        ASR::symbol_t *parent = nullptr;
        switch (dt->type) {
            case (ASR::symbolType::StructType): {
                dt = ASRUtils::symbol_get_past_external(dt);
                ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(dt);
                parent = der_type->m_parent;
                break;
            }
            default :
                require(false,
                    "m_dt::m_v::m_type must point to a Struct type",
                    loc);
        }
        return parent;
    }

    ASR::symbol_t *get_parent_type_dt(ASR::expr_t *dt, const Location &loc) {
        require(ASR::is_a<ASR::Var_t>(*dt),
            "m_dt must point to a Var",
            loc);
        ASR::Var_t *var = ASR::down_cast<ASR::Var_t>(dt);
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(var->m_v);
        ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(v->m_type);
        ASR::symbol_t *type_sym=nullptr;
        ASR::symbol_t *parent = nullptr;
        switch (t2->type) {
            case (ASR::ttypeType::Struct): {
                type_sym = ASR::down_cast<ASR::Struct_t>(t2)->m_derived_type;
                type_sym = ASRUtils::symbol_get_past_external(type_sym);
                ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(type_sym);
                parent = der_type->m_parent;
                break;
            }
            case (ASR::ttypeType::Class): {
                type_sym = ASR::down_cast<ASR::Class_t>(t2)->m_class_type;
                type_sym = ASRUtils::symbol_get_past_external(type_sym);
                if( type_sym->type == ASR::symbolType::StructType ) {
                    ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(type_sym);
                    parent = der_type->m_parent;
                }
                break;
            }
            default :
                require(false,
                    "m_dt::m_v::m_type must point to a Struct type",
                    loc);
        }
        return parent;
    }

    void visit_FunctionCall(const FunctionCall_t &x) {
        require(x.m_name,
            "FunctionCall::m_name must be present",
            x.base.base.loc);
        if (x.m_dt) {
            SymbolTable *symtab = get_dt_symtab(x.m_dt, x.base.base.loc);
            require(symtab_in_scope(symtab, x.m_name),
                "FunctionCall::m_name cannot point outside of its symbol table",
                x.base.base.loc);
        } else {
            require(symtab_in_scope(current_symtab, x.m_name),
                "FunctionCall::m_name `" + std::string(symbol_name(x.m_name)) +
                "` cannot point outside of its symbol table",
                x.base.base.loc);
            // Check both `name` and `orig_name` that `orig_name` points
            // to GenericProcedure (if applicable), both external and non
            // external
            if (check_external) {
                const ASR::symbol_t *fn = ASRUtils::symbol_get_past_external(x.m_name);
                require(ASR::is_a<ASR::Function_t>(*fn),
                    "FunctionCall::m_name must be a Function",
                    x.base.base.loc);
            }
        }
        for (size_t i=0; i<x.n_args; i++) {
            if( x.m_args[i].m_value ) {
                visit_expr(*(x.m_args[i].m_value));
            }
        }
        visit_ttype(*x.m_type);
    }

    void visit_Struct(const Struct_t &x) {
        require(symtab_in_scope(current_symtab, x.m_derived_type),
            "Struct::m_derived_type cannot point outside of its symbol table");
        for (size_t i=0; i<x.n_dims; i++) {
            visit_dimension(x.m_dims[i]);
        }
    }

    /*
    void visit_Pointer(const Pointer_t &x) {
        visit_ttype(*x.m_type);
    }
    */

};


} // namespace ASR

bool asr_verify(const ASR::TranslationUnit_t &unit, bool check_external) {
    ASR::VerifyVisitor v(check_external);
    v.visit_TranslationUnit(unit);
    return true;
}

} // namespace LFortran
