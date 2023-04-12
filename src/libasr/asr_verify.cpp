#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/utils.h>

namespace {
    class VerifyAbort
    {
    };
}

namespace LCompilers {

namespace ASR {

using ASRUtils::symbol_name;
using ASRUtils::symbol_parent_symtab;

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
    bool check_external;
    diag::Diagnostics &diagnostics;

    // For checking that all symtabs have a unique ID.
    // We first walk all symtabs, and then we check that everything else
    // points to them (i.e., that nothing points to some symbol table that
    // is not part of this ASR).
    std::map<uint64_t,SymbolTable*> id_symtab_map;
    std::vector<std::string> function_dependencies;
    std::vector<std::string> module_dependencies;
    std::vector<std::string> variable_dependencies;

    std::set<std::pair<uint64_t, std::string>> const_assigned;

public:
    VerifyVisitor(bool check_external, diag::Diagnostics &diagnostics) : check_external{check_external},
        diagnostics{diagnostics} {}

    // Requires the condition `cond` to be true. Raise an exception otherwise.
#define require(cond, error_msg) require_impl((cond), (error_msg), x.base.base.loc)
#define require_with_loc(cond, error_msg, loc) require_impl((cond), (error_msg), loc)
    void require_impl(bool cond, const std::string &error_msg, const Location &loc) {
        if (!cond) {
            diagnostics.message_label("ASR verify: " + error_msg,
                {loc}, "failed here",
                diag::Level::Error, diag::Stage::ASRVerify);
            throw VerifyAbort();
        }
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

    void visit_BlockCall(const BlockCall_t& x) {
        require(x.m_m != nullptr, "Block call made to inexisting block");
        require(symtab_in_scope(current_symtab, x.m_m),
            "Block " + std::string(ASRUtils::symbol_name(x.m_m)) +
            " should resolve in current scope.");
        SymbolTable *parent_symtab = current_symtab;
        ASR::Block_t* block = ASR::down_cast<ASR::Block_t>(x.m_m);
        LCOMPILERS_ASSERT(block); // already checked above, just making sure
        current_symtab = block->m_symtab;
        for (size_t i=0; i<block->n_body; i++) {
            visit_stmt(*(block->m_body[i]));
        }
        current_symtab = parent_symtab;
    }

    void verify_unique_dependencies(char** m_dependencies,
        size_t n_dependencies, std::string m_name, const Location& loc) {
        // Check if any dependency is duplicated
        // in the dependency list of the function
        std::set<std::string> dependencies_set;
        for( size_t i = 0; i < n_dependencies; i++ ) {
            std::string found_dep = m_dependencies[i];
            require_with_loc(dependencies_set.find(found_dep) == dependencies_set.end(),
                    "Symbol " + found_dep + " is duplicated in the dependency "
                    "list of " + m_name, loc);
            dependencies_set.insert(found_dep);
        }
    }

    void visit_Module(const Module_t &x) {
        module_dependencies.clear();
        module_dependencies.reserve(x.n_dependencies);
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

        verify_unique_dependencies(x.m_dependencies, x.n_dependencies,
                                   x.m_name, x.base.base.loc);

        for (size_t i=0; i < x.n_dependencies; i++) {
            require(x.m_dependencies[i] != nullptr,
                "A module dependency must not be a nullptr");
            require(std::string(x.m_dependencies[i]) != "",
                "A module dependency must not be an empty string");
            require(valid_name(x.m_dependencies[i]),
                "A module dependency must be a valid string");
        }
        for( auto& dep: module_dependencies ) {
            if( dep != x.m_name ) {
                require(present(x.m_dependencies, x.n_dependencies, dep),
                        "Module " + std::string(x.m_name) +
                        " dependencies must contain " + dep +
                        " because a function present in it is getting called in "
                        + std::string(x.m_name) + ".");
            }
        }
        current_symtab = parent_symtab;
    }

    void visit_Assignment(const Assignment_t& x) {
        ASR::expr_t* target = x.m_target;
        if( ASR::is_a<ASR::Var_t>(*target) ) {
            ASR::Var_t* target_Var = ASR::down_cast<ASR::Var_t>(target);
            ASR::ttype_t* target_type = nullptr;
            if( ASR::is_a<ASR::Variable_t>(*target_Var->m_v) ||
                (ASR::is_a<ASR::ExternalSymbol_t>(*target_Var->m_v) &&
                 ASR::down_cast<ASR::ExternalSymbol_t>(target_Var->m_v)->m_external) ) {
                target_type = ASRUtils::expr_type(target);
            }
            if( target_type && ASR::is_a<ASR::Const_t>(*target_type) ) {
                std::string variable_name = ASRUtils::symbol_name(target_Var->m_v);
                require(const_assigned.find(std::make_pair(current_symtab->counter,
                    variable_name)) == const_assigned.end(),
                    "Assignment target with " + ASRUtils::type_to_str_python(target_type)
                    + " cannot be re-assigned.");
                const_assigned.insert(std::make_pair(current_symtab->counter, variable_name));
            }
        }
        BaseWalkVisitor<VerifyVisitor>::visit_Assignment(x);
    }

    void visit_ClassProcedure(const ClassProcedure_t &x) {
        require(x.m_name != nullptr,
            "The ClassProcedure::m_name cannot be nullptr");
        require(x.m_proc != nullptr,
            "The ClassProcedure::m_proc cannot be nullptr");
        require(x.m_proc_name != nullptr,
            "The ClassProcedure::m_proc_name cannot be nullptr");

        SymbolTable *symtab = x.m_parent_symtab;
        require(symtab != nullptr,
            "ClassProcedure::m_parent_symtab cannot be nullptr");
        require(symtab->get_symbol(std::string(x.m_name)) != nullptr,
            "ClassProcedure '" + std::string(x.m_name) + "' not found in parent_symtab symbol table");
        symbol_t *symtab_sym = symtab->get_symbol(std::string(x.m_name));
        const symbol_t *current_sym = &x.base;
        require(symtab_sym == current_sym,
            "ClassProcedure's parent symbol table does not point to it");
        require(id_symtab_map.find(symtab->counter) != id_symtab_map.end(),
            "ClassProcedure::m_parent_symtab must be present in the ASR ("
                + std::string(x.m_name) + ")");

        ASR::Function_t* x_m_proc = ASR::down_cast<ASR::Function_t>(x.m_proc);
        if( x.m_self_argument ) {
            bool arg_found = false;
            std::string self_arg_name = std::string(x.m_self_argument);
            for( size_t i = 0; i < x_m_proc->n_args; i++ ) {
                std::string arg_name = std::string(ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Var_t>(x_m_proc->m_args[i])->m_v));
                if( self_arg_name == arg_name ) {
                    arg_found = true;
                    break ;
                }
            }
            require(arg_found, self_arg_name + " must be present in " +
                    std::string(x.m_name) + " procedures.");
        }
    }

    void visit_Function(const Function_t &x) {
        std::vector<std::string> function_dependencies_copy = function_dependencies;
        function_dependencies.clear();
        function_dependencies.reserve(x.n_dependencies);
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

        verify_unique_dependencies(x.m_dependencies, x.n_dependencies,
                                   x.m_name, x.base.base.loc);

        // Check if there are unnecessary dependencies
        // present in the dependency list of the function
        for( size_t i = 0; i < x.n_dependencies; i++ ) {
            std::string found_dep = x.m_dependencies[i];
            require(std::find(function_dependencies.begin(), function_dependencies.end(), found_dep) != function_dependencies.end(),
                    "Function " + std::string(x.m_name) + " doesn't depend on " + found_dep +
                    " but is found in its dependency list.");
        }

        // Check if all the dependencies found are
        // present in the dependency list of the function
        for( auto& found_dep: function_dependencies ) {
            require(present(x.m_dependencies, x.n_dependencies, found_dep),
                    "Function " + std::string(x.m_name) + " depends on " + found_dep +
                    " but isn't found in its dependency list.");
        }
        current_symtab = parent_symtab;
        function_dependencies = function_dependencies_copy;
    }

    template <typename T>
    void visit_UserDefinedType(const T &x) {
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
        std::vector<std::string> struct_dependencies;
        for (auto &a : x.m_symtab->get_scope()) {
            this->visit_symbol(*a.second);
            if( ASR::is_a<ASR::ClassProcedure_t>(*a.second) ||
                ASR::is_a<ASR::GenericProcedure_t>(*a.second) ||
                ASR::is_a<ASR::StructType_t>(*a.second) ||
                ASR::is_a<ASR::UnionType_t>(*a.second) ||
                ASR::is_a<ASR::ExternalSymbol_t>(*a.second) ||
                ASR::is_a<ASR::CustomOperator_t>(*a.second) ) {
                continue ;
            }
            ASR::ttype_t* var_type = ASRUtils::type_get_past_pointer(ASRUtils::symbol_type(a.second));
            char* aggregate_type_name = nullptr;
            ASR::symbol_t* sym = nullptr;
            if( ASR::is_a<ASR::Struct_t>(*var_type) ) {
                sym = ASR::down_cast<ASR::Struct_t>(var_type)->m_derived_type;
                aggregate_type_name = ASRUtils::symbol_name(sym);
            } else if( ASR::is_a<ASR::Enum_t>(*var_type) ) {
                sym = ASR::down_cast<ASR::Enum_t>(var_type)->m_enum_type;
                aggregate_type_name = ASRUtils::symbol_name(sym);
            } else if( ASR::is_a<ASR::Union_t>(*var_type) ) {
                sym = ASR::down_cast<ASR::Union_t>(var_type)->m_union_type;
                aggregate_type_name = ASRUtils::symbol_name(sym);
            } else if( ASR::is_a<ASR::Class_t>(*var_type) ) {
                sym = ASR::down_cast<ASR::Class_t>(var_type)->m_class_type;
                aggregate_type_name = ASRUtils::symbol_name(sym);
            }
            if( aggregate_type_name && ASRUtils::symbol_parent_symtab(sym) != current_symtab ) {
                struct_dependencies.push_back(std::string(aggregate_type_name));
                require(present(x.m_dependencies, x.n_dependencies, std::string(aggregate_type_name)),
                    std::string(x.m_name) + " depends on " + std::string(aggregate_type_name)
                    + " but it isn't found in its dependency list.");
            }
        }
        for( size_t i = 0; i < x.n_dependencies; i++ ) {
            require(std::find(struct_dependencies.begin(), struct_dependencies.end(),
                    std::string(x.m_dependencies[i])) != struct_dependencies.end(),
                std::string(x.m_dependencies[i]) + " is not a dependency of " + std::string(x.m_name)
                + " but it is present in its dependency list.");
        }

        verify_unique_dependencies(x.m_dependencies, x.n_dependencies,
                                   x.m_name, x.base.base.loc);
        current_symtab = parent_symtab;
    }

    void visit_StructType(const StructType_t& x) {
        visit_UserDefinedType(x);
        if( !x.m_alignment ) {
            return ;
        }
        ASR::expr_t* aligned_expr_value = ASRUtils::expr_value(x.m_alignment);
        std::string msg = "Alignment should always evaluate to a constant expressions.";
        require(aligned_expr_value, msg);
        int64_t alignment_int;
        require(ASRUtils::extract_value(aligned_expr_value, alignment_int), msg);
        require(alignment_int != 0 && (alignment_int & (alignment_int - 1)) == 0,
                "Alignment " + std::to_string(alignment_int) +
                " is not a positive power of 2.");
    }

    void visit_EnumType(const EnumType_t& x) {
        visit_UserDefinedType(x);
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
        visit_UserDefinedType(x);
    }

    void visit_Variable(const Variable_t &x) {
        variable_dependencies.clear();
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

        ASR::asr_t* asr_owner = symtab->asr_owner;
        bool is_module = false;
        if( ASR::is_a<ASR::symbol_t>(*asr_owner) &&
            ASR::is_a<ASR::Module_t>(
                *ASR::down_cast<ASR::symbol_t>(asr_owner)) ) {
            is_module = true;
        }
        if( symtab->parent != nullptr &&
            !is_module) {
            // For now restrict this check only to variables which are present
            // inside symbols which have a body.
            require( (x.m_symbolic_value == nullptr && x.m_value == nullptr) ||
                     (x.m_symbolic_value != nullptr && x.m_value != nullptr) ||
                     (x.m_symbolic_value != nullptr && ASRUtils::is_value_constant(x.m_symbolic_value)),
                    "Initialisation of " + std::string(x.m_name) +
                    " must reduce to a compile time constant.");
        }

        if (x.m_symbolic_value)
            visit_expr(*x.m_symbolic_value);
        visit_ttype(*x.m_type);

        verify_unique_dependencies(x.m_dependencies, x.n_dependencies,
                                   x.m_name, x.base.base.loc);

        // Verify dependencies
        for( size_t i = 0; i < x.n_dependencies; i++ ) {
            require(std::find(
                variable_dependencies.begin(),
                variable_dependencies.end(),
                std::string(x.m_dependencies[i])
            ) != variable_dependencies.end(),
                "Variable " + std::string(x.m_name) + " doesn't depend on " +
                std::string(x.m_dependencies[i]) + " but is found in its dependency list.");
        }

        for( size_t i = 0; i < variable_dependencies.size(); i++ ) {
            require(present(x.m_dependencies, x.n_dependencies, variable_dependencies[i]),
                "Variable " + std::string(x.m_name) + " depends on " +
                std::string(variable_dependencies[i]) + " but isn't found in its dependency list.");
        }
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
            ASR::StructType_t* sm = nullptr;
            ASR::EnumType_t* em = nullptr;
            bool is_valid_owner = false;
            is_valid_owner = m != nullptr && ((ASR::symbol_t*) m == ASRUtils::get_asr_owner(x.m_external));
            std::string asr_owner_name = "";
            if( !is_valid_owner ) {
                ASR::symbol_t* asr_owner_sym = ASRUtils::get_asr_owner(x.m_external);
                is_valid_owner = (ASR::is_a<ASR::StructType_t>(*asr_owner_sym) ||
                                  ASR::is_a<ASR::EnumType_t>(*asr_owner_sym));
                if( ASR::is_a<ASR::StructType_t>(*asr_owner_sym) ) {
                    sm = ASR::down_cast<ASR::StructType_t>(asr_owner_sym);
                    asr_owner_name = sm->m_name;
                } else if( ASR::is_a<ASR::EnumType_t>(*asr_owner_sym) ) {
                    em = ASR::down_cast<ASR::EnumType_t>(asr_owner_sym);
                    asr_owner_name = em->m_name;
                }
            } else {
                asr_owner_name = m->m_name;
            }
            std::string x_m_module_name = x.m_module_name;
            if( current_symtab->resolve_symbol(x.m_module_name) ) {
                x_m_module_name = ASRUtils::symbol_name(
                    ASRUtils::symbol_get_past_external(
                        current_symtab->resolve_symbol(x.m_module_name)));
            }
            require(is_valid_owner,
                "ExternalSymbol::m_external is not in a module or struct type");
            require(x_m_module_name == asr_owner_name,
                "ExternalSymbol::m_module_name `" + x_m_module_name
                + "` must match external's module name `" + asr_owner_name + "`");
            ASR::symbol_t *s = nullptr;
            if( m != nullptr && ((ASR::symbol_t*) m == ASRUtils::get_asr_owner(x.m_external)) ) {
                s = m->m_symtab->find_scoped_symbol(x.m_original_name, x.n_scope_names, x.m_scope_names);
            } else if( sm ) {
                s = sm->m_symtab->resolve_symbol(std::string(x.m_original_name));
            } else if( em ) {
                s = em->m_symtab->resolve_symbol(std::string(x.m_original_name));
            }
            require(s != nullptr,
                "ExternalSymbol::m_original_name ('"
                + std::string(x.m_original_name)
                + "') + scope_names not found in a module '"
                + asr_owner_name + "'");
            require(s == x.m_external,
                std::string("ExternalSymbol::m_name + scope_names found but not equal to m_external, ") +
                "original_name " + std::string(x.m_original_name) + ".");
        }
    }

    // --------------------------------------------------------
    // nodes that have symbol in their fields:

    void visit_Var(const Var_t &x) {
        require(x.m_v != nullptr,
            "Var_t::m_v cannot be nullptr");
        std::string x_mv_name = ASRUtils::symbol_name(x.m_v);
        require(is_a<Variable_t>(*x.m_v) || is_a<ExternalSymbol_t>(*x.m_v)
                || is_a<Function_t>(*x.m_v) || is_a<ASR::EnumType_t>(*x.m_v),
            "Var_t::m_v " + x_mv_name + " does not point to a Variable_t, ExternalSymbol_t, " \
            "Function_t, Subroutine_t or EnumType_t");

        require(symtab_in_scope(current_symtab, x.m_v),
            "Var::m_v `" + x_mv_name + "` cannot point outside of its symbol table");
        variable_dependencies.push_back(x_mv_name);
    }

    void visit_ImplicitDeallocate(const ImplicitDeallocate_t &x) {
        // TODO: check that every allocated variable is deallocated.
        BaseWalkVisitor::visit_ImplicitDeallocate(x);
    }

    void check_var_external(const ASR::expr_t &x) {
        if (ASR::is_a<ASR::Var_t>(x)) {
            ASR::symbol_t *s = ((ASR::Var_t*)&x)->m_v;
            if (ASR::is_a<ASR::ExternalSymbol_t>(*s)) {
                ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(s);
                require_impl(e->m_external, "m_external cannot be null here",
                        x.base.loc);
            }
        }
    }

    template <typename T>
    void handle_ArrayItemSection(const T &x) {
        visit_expr(*x.m_v);
        for (size_t i=0; i<x.n_args; i++) {
            visit_array_index(x.m_args[i]);
        }
        require(x.m_type != nullptr,
            "ArrayItemSection::m_type cannot be nullptr");
        visit_ttype(*x.m_type);
        if (check_external) {
            check_var_external(*x.m_v);
            int n_dims = ASRUtils::extract_n_dims_from_ttype(
                    ASRUtils::expr_type(x.m_v));
            if (ASR::is_a<ASR::Character_t>(*x.m_type) && n_dims == 0) {
                // TODO: This seems like a bug, we should not use ArrayItem with
                // strings but StringItem. For now we ignore it, but we should
                // fix it
            } else {
                require(n_dims > 0,
                    "The variable in ArrayItem must be an array, not a scalar");
            }
        }
    }

    void visit_ArrayItem(const ArrayItem_t &x) {
        handle_ArrayItemSection(x);
    }

    void visit_ArraySection(const ArraySection_t &x) {
        handle_ArrayItemSection(x);
    }

    template <typename T>
    void verify_args(const T& x) {
        ASR::symbol_t* func_sym = ASRUtils::symbol_get_past_external(x.m_name);
        ASR::Function_t* func = nullptr;
        if( func_sym && ASR::is_a<ASR::Function_t>(*func_sym) ) {
            func = ASR::down_cast<ASR::Function_t>(func_sym);
        }

        if( func ) {
            for (size_t i=0; i<x.n_args; i++) {
                ASR::symbol_t* arg_sym = ASR::down_cast<ASR::Var_t>(func->m_args[i])->m_v;
                if (x.m_args[i].m_value == nullptr &&
                    (ASR::is_a<ASR::Variable_t>(*arg_sym) &&
                        ASR::down_cast<ASR::Variable_t>(arg_sym)->m_presence !=
                        ASR::presenceType::Optional)) {

                        require(false, "Required argument " +
                                    std::string(ASRUtils::symbol_name(arg_sym)) +
                                    " cannot be nullptr.");

                }
            }
        }

        for (size_t i=0; i<x.n_args; i++) {
            if( x.m_args[i].m_value ) {
                visit_expr(*(x.m_args[i].m_value));
            }
        }
    }

    void visit_SubroutineCall(const SubroutineCall_t &x) {
        require(symtab_in_scope(current_symtab, x.m_name),
            "SubroutineCall::m_name '" + std::string(symbol_name(x.m_name)) + "' cannot point outside of its symbol table");
        if (check_external) {
            ASR::symbol_t *s = ASRUtils::symbol_get_past_external(x.m_name);
            require(ASR::is_a<ASR::Function_t>(*s) ||
                    ASR::is_a<ASR::ClassProcedure_t>(*s),
                "SubroutineCall::m_name '" + std::string(symbol_name(x.m_name)) + "' must be a Function or ClassProcedure.");
        }

        function_dependencies.push_back(std::string(ASRUtils::symbol_name(x.m_name)));
        if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) ) {
            ASR::ExternalSymbol_t* x_m_name = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
            if( x_m_name->m_external && ASR::is_a<ASR::Module_t>(*ASRUtils::get_asr_owner(x_m_name->m_external)) ) {
                module_dependencies.push_back(std::string(x_m_name->m_module_name));
            }
        }

        verify_args(x);
    }

    SymbolTable *get_dt_symtab(ASR::symbol_t *dt) {
        LCOMPILERS_ASSERT(dt)
        SymbolTable *symtab = ASRUtils::symbol_symtab(ASRUtils::symbol_get_past_external(dt));
        require_impl(symtab,
            "m_dt::m_v::m_type::class/derived_type must point to a symbol with a symbol table",
            dt->base.loc);
        return symtab;
    }

    SymbolTable *get_dt_symtab(ASR::expr_t *dt) {
        ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(dt));
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
                require_impl(false,
                    "m_dt::m_v::m_type must point to a type with a symbol table (Struct or Class)",
                    dt->base.loc);
        }
        return get_dt_symtab(type_sym);
    }

    ASR::symbol_t *get_parent_type_dt(ASR::symbol_t *dt) {
        ASR::symbol_t *parent = nullptr;
        switch (dt->type) {
            case (ASR::symbolType::StructType): {
                dt = ASRUtils::symbol_get_past_external(dt);
                ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(dt);
                parent = der_type->m_parent;
                break;
            }
            default :
                require_impl(false,
                    "m_dt::m_v::m_type must point to a Struct type",
                    dt->base.loc);
        }
        return parent;
    }

    ASR::symbol_t *get_parent_type_dt(ASR::expr_t *dt) {
        ASR::ttype_t *t2 = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(dt));
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
                require_impl(false,
                    "m_dt::m_v::m_type must point to a Struct type",
                    dt->base.loc);
        }
        return parent;
    }

    void visit_PointerNullConstant(const PointerNullConstant_t& x) {
        require(x.m_type != nullptr, "null() must have a type");
    }

    void visit_FunctionCall(const FunctionCall_t &x) {
        require(x.m_name,
            "FunctionCall::m_name must be present");
        function_dependencies.push_back(std::string(ASRUtils::symbol_name(x.m_name)));
        if( ASR::is_a<ASR::ExternalSymbol_t>(*x.m_name) ) {
            ASR::ExternalSymbol_t* x_m_name = ASR::down_cast<ASR::ExternalSymbol_t>(x.m_name);
            if( x_m_name->m_external && ASR::is_a<ASR::Module_t>(*ASRUtils::get_asr_owner(x_m_name->m_external)) ) {
                module_dependencies.push_back(std::string(x_m_name->m_module_name));
            }
        }

        require(symtab_in_scope(current_symtab, x.m_name),
            "FunctionCall::m_name `" + std::string(symbol_name(x.m_name)) +
            "` cannot point outside of its symbol table");
        // Check both `name` and `orig_name` that `orig_name` points
        // to GenericProcedure (if applicable), both external and non
        // external
        if (check_external) {
            const ASR::symbol_t *fn = ASRUtils::symbol_get_past_external(x.m_name);
            require(ASR::is_a<ASR::Function_t>(*fn) ||
                    (ASR::is_a<ASR::Variable_t>(*fn) &&
                    ASR::is_a<ASR::FunctionType_t>(*ASRUtils::symbol_type(fn))) ||
                    ASR::is_a<ASR::ClassProcedure_t>(*fn),
                "FunctionCall::m_name must be a Function or Variable with FunctionType");
        }

        verify_args(x);
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

bool asr_verify(const ASR::TranslationUnit_t &unit, bool check_external,
            diag::Diagnostics &diagnostics) {
    ASR::VerifyVisitor v(check_external, diagnostics);
    try {
        v.visit_TranslationUnit(unit);
    } catch (const VerifyAbort &) {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return false;
    }
    return true;
}

} // namespace LCompilers
