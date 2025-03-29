#include <unordered_set>
#include <map>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/serialization.h>
#include <libasr/assert.h>
#include <libasr/asr_verify.h>
#include <libasr/utils.h>
#include <libasr/modfile.h>
#include <libasr/pass/pass_utils.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_subroutine_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>

#include <libasr/asr_builder.h>

namespace LCompilers {

    namespace ASRUtils  {

// depth-first graph traversal
void visit(
    std::string const& a,
    std::map<std::string, std::vector<std::string>> const& deps,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& result
) {
    visited.insert(a);
    auto it = deps.find(a);
    if (it != deps.end()) {
        for (auto n : it->second) {
            if (!visited.count(n)) visit(n, deps, visited, result);
        }
    }
    result.push_back(a);
}

std::vector<std::string> order_deps(std::map<std::string, std::vector<std::string>> const& deps) {
    // Compute ordering: depth-first graph traversal, inserting nodes on way back

    // set containing the visited nodes
    std::unordered_set<std::string> visited;

    // vector containing result
    std::vector<std::string> result;

    for (auto d : deps) {
        if (!visited.count(d.first)) {
            visit(d.first, deps, visited, result);
        }
    }
    return result;
}

std::vector<std::string> determine_module_dependencies(
        const ASR::TranslationUnit_t &unit)
{
    std::map<std::string, std::vector<std::string>> deps;
    for (auto &item : unit.m_symtab->get_scope()) {
        if (ASR::is_a<ASR::Module_t>(*item.second)) {
            std::string name = item.first;
            ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(item.second);
            deps[name] = std::vector<std::string>();
            for (size_t i=0; i < m->n_dependencies; i++) {
                std::string dep = m->m_dependencies[i];
                deps[name].push_back(dep);
            }
        }
    }
    return order_deps(deps);
}

std::vector<std::string> determine_function_definition_order(
        SymbolTable* symtab) {
    std::map<std::string, std::vector<std::string>> func_dep_graph;
    ASR::symbol_t *sym;
    for( auto itr: symtab->get_scope() ) {
        if( ASR::is_a<ASR::Function_t>(*itr.second) ) {
            std::vector<std::string> deps;
            ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(itr.second);
            for( size_t i = 0; i < func->n_dependencies; i++ ) {
                std::string dep = func->m_dependencies[i];
                // Check if the dependent variable is present in the symtab.
                // This will help us to include only local dependencies, and we
                // assume that dependencies in the parent symtab are already declared
                // earlier.
                sym = symtab->get_symbol(dep);
                if (sym != nullptr && ASR::is_a<ASR::Function_t>(*sym))
                    deps.push_back(dep);
            }
            func_dep_graph[itr.first] = deps;
        }
    }
    return ASRUtils::order_deps(func_dep_graph);
}

std::vector<std::string> determine_variable_declaration_order(
         SymbolTable* symtab) {
    std::map<std::string, std::vector<std::string>> var_dep_graph;
    for( auto itr: symtab->get_scope() ) {
        if( ASR::is_a<ASR::Variable_t>(*itr.second) ) {
            std::vector<std::string> deps;
            ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(itr.second);
            for( size_t i = 0; i < var->n_dependencies; i++ ) {
                std::string dep = var->m_dependencies[i];
                // Check if the dependent variable is present in the symtab.
                // This will help us to include only local dependencies, and we
                // assume that dependencies in the parent symtab are already declared
                // earlier.
                if (symtab->get_symbol(dep) != nullptr)
                    deps.push_back(dep);
            }
            var_dep_graph[itr.first] = deps;
        }
    }
    return ASRUtils::order_deps(var_dep_graph);
}

void extract_module_python(const ASR::TranslationUnit_t &m,
                std::vector<std::pair<std::string, ASR::Module_t*>>& children_modules,
                std::string module_name) {
    bool module_found = false;
    for (auto &a : m.m_symtab->get_scope()) {
        if( ASR::is_a<ASR::Module_t>(*a.second) ) {
            if( a.first == "__main__" ) {
                module_found = true;
                children_modules.push_back(std::make_pair(module_name,
                    ASR::down_cast<ASR::Module_t>(a.second)));
            } else {
                children_modules.push_back(std::make_pair(a.first,
                    ASR::down_cast<ASR::Module_t>(a.second)));
            }
        }
    }
    if( !module_found ) {
        throw LCompilersException("ICE: Module not found");
    }
}

void update_call_args(Allocator &al, SymbolTable *current_scope, bool implicit_interface,
        std::map<std::string, ASR::symbol_t*> changed_external_function_symbol) {
    /*
        Iterate over body of program, check if there are any subroutine calls if yes, iterate over its args
        and update the args if they are equal to the old symbol
        For example:
            function func(f)
                double precision c
                call sub2(c)
                print *, c(d)
            end function
        This function updates `sub2` to use the new symbol `c` that is now a function, not a variable.
        Along with this, it also updates the args of `sub2` to use the new symbol `c` instead of the old one.
    */

    class ArgsReplacer : public ASR::BaseExprReplacer<ArgsReplacer> {
    public:
        Allocator &al;
        ASR::symbol_t* new_sym;

        ArgsReplacer(Allocator &al_) : al(al_) {}

        void replace_Var(ASR::Var_t* x) {
            *current_expr = ASRUtils::EXPR(ASR::make_Var_t(al, x->base.base.loc, new_sym));
        }
    };

    class ArgsVisitor : public ASR::CallReplacerOnExpressionsVisitor<ArgsVisitor>
    {
        public:
        Allocator &al;
        SymbolTable* scope = current_scope;
        ArgsReplacer replacer;
        std::map<std::string, ASR::symbol_t*> &changed_external_function_symbol;
        ArgsVisitor(Allocator &al_, std::map<std::string, ASR::symbol_t*> &changed_external_function_symbol_) : al(al_), replacer(al_),
                    changed_external_function_symbol(changed_external_function_symbol_) {}

        void call_replacer_(ASR::symbol_t* new_sym) {
            replacer.current_expr = current_expr;
            replacer.new_sym = new_sym;
            replacer.replace_expr(*current_expr);
        }

        ASR::symbol_t* fetch_sym(ASR::symbol_t* arg_sym_underlying) {
            ASR::symbol_t* sym = nullptr;
            if (ASR::is_a<ASR::Variable_t>(*arg_sym_underlying)) {
                ASR::Variable_t* arg_variable = ASR::down_cast<ASR::Variable_t>(arg_sym_underlying);
                std::string arg_variable_name = std::string(arg_variable->m_name);
                sym = arg_variable->m_parent_symtab->get_symbol(arg_variable_name);
            } else if (ASR::is_a<ASR::Function_t>(*arg_sym_underlying)) {
                ASR::Function_t* arg_function = ASR::down_cast<ASR::Function_t>(arg_sym_underlying);
                std::string arg_function_name = std::string(arg_function->m_name);
                sym = arg_function->m_symtab->parent->get_symbol(arg_function_name);
            }
            return sym;
        }

        void handle_Var(ASR::expr_t* arg_expr, ASR::expr_t** expr_to_replace) {
            if (arg_expr && ASR::is_a<ASR::Var_t>(*arg_expr)) {
                ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(arg_expr);
                ASR::symbol_t* arg_sym = arg_var->m_v;
                ASR::symbol_t* arg_sym_underlying = ASRUtils::symbol_get_past_external(arg_sym);
                ASR::symbol_t* sym = fetch_sym(arg_sym_underlying);
                if (sym != arg_sym_underlying) {
                    ASR::expr_t** current_expr_copy = current_expr;
                    current_expr = const_cast<ASR::expr_t**>((expr_to_replace));
                    this->call_replacer_(sym);
                    current_expr = current_expr_copy;
                }
            }
        }


        void visit_SubroutineCall(const ASR::SubroutineCall_t& x) {
            ASR::SubroutineCall_t* subrout_call = (ASR::SubroutineCall_t*)(&x);
            for (size_t j = 0; j < subrout_call->n_args; j++) {
                ASR::call_arg_t arg = subrout_call->m_args[j];
                ASR::expr_t* arg_expr = arg.m_value;
                handle_Var(arg_expr, &(subrout_call->m_args[j].m_value));
            }
        }

        void visit_FunctionCall(const ASR::FunctionCall_t& x) {
            ASR::FunctionCall_t* func_call = (ASR::FunctionCall_t*)(&x);
            for (size_t j = 0; j < func_call->n_args; j++) {
                ASR::call_arg_t arg = func_call->m_args[j];
                ASR::expr_t* arg_expr = arg.m_value;
                handle_Var(arg_expr, &(func_call->m_args[j].m_value));
            }
        }

        void visit_Function(const ASR::Function_t& x) {
            ASR::Function_t* func = (ASR::Function_t*)(&x);
            scope = func->m_symtab;
            ASRUtils::SymbolDuplicator symbol_duplicator(al);
            std::map<std::string, ASR::symbol_t*> scope_ = scope->get_scope();
            std::vector<std::string> symbols_to_duplicate;
            for (auto it: scope_) {
                if (changed_external_function_symbol.find(it.first) != changed_external_function_symbol.end() &&
                    is_external_sym_changed(it.second, changed_external_function_symbol[it.first])) {
                    symbols_to_duplicate.push_back(it.first);
                }
            }

            for (auto it: symbols_to_duplicate) {
                scope->erase_symbol(it);
                symbol_duplicator.duplicate_symbol(changed_external_function_symbol[it], scope);
            }

            for (size_t i = 0; i < func->n_args; i++) {
                ASR::expr_t* arg_expr = func->m_args[i];
                if (ASR::is_a<ASR::Var_t>(*arg_expr)) {
                    ASR::Var_t* arg_var = ASR::down_cast<ASR::Var_t>(arg_expr);
                    ASR::symbol_t* arg_sym = arg_var->m_v;
                    ASR::symbol_t* arg_sym_underlying = ASRUtils::symbol_get_past_external(arg_sym);
                    ASR::symbol_t* sym = fetch_sym(arg_sym_underlying);
                    if (sym != arg_sym_underlying) {
                        ASR::expr_t** current_expr_copy = current_expr;
                        current_expr = const_cast<ASR::expr_t**>(&(func->m_args[i]));
                        this->call_replacer_(sym);
                        current_expr = current_expr_copy;
                    }
                }
            }
            scope = func->m_symtab;
            for (auto &it: scope->get_scope()) {
                visit_symbol(*it.second);
            }
            scope = func->m_symtab;
            for (size_t i = 0; i < func->n_body; i++) {
                visit_stmt(*func->m_body[i]);
            }
            scope = func->m_symtab;
        }
    };

    if (implicit_interface) {
        ArgsVisitor v(al, changed_external_function_symbol);
        SymbolTable *tu_symtab = ASRUtils::get_tu_symtab(current_scope);
        ASR::asr_t* asr_ = tu_symtab->asr_owner;
        ASR::TranslationUnit_t* tu = ASR::down_cast2<ASR::TranslationUnit_t>(asr_);
        v.visit_TranslationUnit(*tu);
    }
}

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m) {
    LCOMPILERS_ASSERT(m.m_symtab->get_scope().size()== 1);
    for (auto &a : m.m_symtab->get_scope()) {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        return ASR::down_cast<ASR::Module_t>(a.second);
    }
    throw LCompilersException("ICE: Module not found");
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            LCompilers::PassOptions& pass_options,
                            bool run_verify,
                            const std::function<void (const std::string &, const Location &)> err,
                            LCompilers::LocationManager &lm) {
    LCOMPILERS_ASSERT(symtab);
    if (symtab->get_symbol(module_name) != nullptr) {
        ASR::symbol_t *m = symtab->get_symbol(module_name);
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            err("The symbol '" + module_name + "' is not a module", loc);
        }
    }
    LCOMPILERS_ASSERT(symtab->parent == nullptr);
    ASR::TranslationUnit_t *mod1 = find_and_load_module(al, module_name,
            *symtab, intrinsic, pass_options, lm);
    if (mod1 == nullptr && !intrinsic) {
        // Module not found as a regular module. Try intrinsic module
        if (module_name == "iso_c_binding"
            ||module_name == "iso_fortran_env"
            ||module_name == "ieee_arithmetic") {
            mod1 = find_and_load_module(al, "lfortran_intrinsic_" + module_name,
                *symtab, true, pass_options, lm);
        }
    }
    if (mod1 == nullptr) {
        err("Module '" + module_name + "' not declared in the current source and the modfile was not found",
            loc);
    }
    ASR::Module_t *mod2 = extract_module(*mod1);
    symtab->add_symbol(module_name, (ASR::symbol_t*)mod2);
    mod2->m_symtab->parent = symtab;
    mod2->m_loaded_from_mod = true;
    LCOMPILERS_ASSERT(symtab->resolve_symbol(module_name));

    // Create a temporary TranslationUnit just for fixing the symbols
    ASR::asr_t *orig_asr_owner = symtab->asr_owner;
    ASR::TranslationUnit_t *tu
        = ASR::down_cast2<ASR::TranslationUnit_t>(ASR::make_TranslationUnit_t(al, loc,
            symtab, nullptr, 0));

    // Load any dependent modules recursively
    bool rerun = true;
    while (rerun) {
        rerun = false;
        std::vector<std::string> modules_list
            = determine_module_dependencies(*tu);
        for (auto &item : modules_list) {
            if (symtab->get_symbol(item)
                    == nullptr) {
                // A module that was loaded requires to load another
                // module

                // This is not very robust, we should store that information
                // in the ASR itself, or encode in the name in a robust way,
                // such as using `module_name@intrinsic`:
                bool is_intrinsic = startswith(item, "lfortran_intrinsic");
                ASR::TranslationUnit_t *mod1 = find_and_load_module(al,
                        item,
                        *symtab, is_intrinsic, pass_options, lm);
                if (mod1 == nullptr && !is_intrinsic) {
                    // Module not found as a regular module. Try intrinsic module
                    if (item == "iso_c_binding"
                        ||item == "iso_fortran_env") {
                        mod1 = find_and_load_module(al, "lfortran_intrinsic_" + item,
                            *symtab, true, pass_options, lm);
                    }
                }

                if (mod1 == nullptr) {
                    err("Module '" + item + "' modfile was not found", loc);
                }
                ASR::Module_t *mod2 = extract_module(*mod1);
                symtab->add_symbol(item, (ASR::symbol_t*)mod2);
                mod2->m_symtab->parent = symtab;
                mod2->m_loaded_from_mod = true;
                rerun = true;
            }
        }
    }

    // Check that all modules are included in ASR now
    std::vector<std::string> modules_list
        = determine_module_dependencies(*tu);
    for (auto &item : modules_list) {
        if (symtab->get_symbol(item) == nullptr) {
            err("ICE: Module '" + item + "' modfile was not found, but should have", loc);
        }
    }

    // Fix all external symbols
    fix_external_symbols(*tu, *symtab);
    PassUtils::UpdateDependenciesVisitor v(al);
    v.visit_TranslationUnit(*tu);
    if (run_verify) {
#if defined(WITH_LFORTRAN_ASSERT)
        diag::Diagnostics diagnostics;
        if (!asr_verify(*tu, true, diagnostics)) {
            std::cerr << diagnostics.render2();
            throw LCompilersException("Verify failed");
        };
#endif
    }
    symtab->asr_owner = orig_asr_owner;

    return mod2;
}

void set_intrinsic(ASR::symbol_t* sym) {
    switch( sym->type ) {
        case ASR::symbolType::Module: {
            ASR::Module_t* module_sym = ASR::down_cast<ASR::Module_t>(sym);
            module_sym->m_intrinsic = true;
            for( auto& itr: module_sym->m_symtab->get_scope() ) {
                set_intrinsic(itr.second);
            }
            break;
        }
        case ASR::symbolType::Function: {
            ASR::Function_t* function_sym = ASR::down_cast<ASR::Function_t>(sym);
            ASR::FunctionType_t* func_sym_type = ASR::down_cast<ASR::FunctionType_t>(function_sym->m_function_signature);
            func_sym_type->m_abi = ASR::abiType::Intrinsic;
            break;
        }
        case ASR::symbolType::Struct: {
            ASR::Struct_t* derived_type_sym = ASR::down_cast<ASR::Struct_t>(sym);
            derived_type_sym->m_abi = ASR::abiType::Intrinsic;
            break;
        }
        case ASR::symbolType::Variable: {
            ASR::Variable_t* derived_type_sym = ASR::down_cast<ASR::Variable_t>(sym);
            derived_type_sym->m_abi = ASR::abiType::Intrinsic;
            break;
        }
        default: {
            break;
        }
    }
}

void set_intrinsic(ASR::TranslationUnit_t* trans_unit) {
    for( auto& itr: trans_unit->m_symtab->get_scope() ) {
        set_intrinsic(itr.second);
    }
}

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                LCompilers::PassOptions& pass_options,
                                                LCompilers::LocationManager &lm) {
    std::filesystem::path runtime_library_dir { pass_options.runtime_library_dir };
    std::filesystem::path filename {msym + ".mod"};
    std::vector<std::filesystem::path> mod_files_dirs;

    mod_files_dirs.push_back( runtime_library_dir );
    mod_files_dirs.push_back( pass_options.mod_files_dir );
    mod_files_dirs.insert(mod_files_dirs.end(),
                          pass_options.include_dirs.begin(),
                          pass_options.include_dirs.end());

    for (auto path : mod_files_dirs) {
        std::string modfile;
        std::filesystem::path full_path = path / filename;
        if (read_file(full_path.string(), modfile)) {
            ASR::TranslationUnit_t *asr = load_modfile(al, modfile, false, symtab, lm);
            if (intrinsic) {
                set_intrinsic(asr);
            }
            return asr;
        }
    }
    return nullptr;
}

ASR::asr_t* getStructInstanceMember_t(Allocator& al, const Location& loc,
                            ASR::asr_t* v_var, ASR::symbol_t *v,
                            ASR::symbol_t* member, SymbolTable* current_scope) {
    member = ASRUtils::symbol_get_past_external(member);
    if (ASR::is_a<ASR::Struct_t>(*member)) {
        ASR::Struct_t* member_variable = ASR::down_cast<ASR::Struct_t>(member);
        ASR::symbol_t *mem_es = nullptr;
        std::string mem_name = "1_" + std::string(ASRUtils::symbol_name(member));
        if (current_scope->resolve_symbol(mem_name)) {
            mem_es = current_scope->resolve_symbol(mem_name);
        } else {
            mem_es = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                member->base.loc, current_scope, s2c(al, mem_name), member,
                ASRUtils::symbol_name(ASRUtils::get_asr_owner(member)),
                nullptr, 0, member_variable->m_name, ASR::accessType::Public));
            current_scope->add_symbol(mem_name, mem_es);
        }
        ASR::ttype_t* member_type = ASRUtils::TYPE(ASR::make_StructType_t(al,
            member_variable->base.base.loc, mem_es));
        return ASR::make_StructInstanceMember_t(al, loc, ASRUtils::EXPR(v_var),
            mem_es, ASRUtils::fix_scoped_type(al, member_type, current_scope), nullptr);
    } else {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
        ASR::Variable_t* member_variable = ASR::down_cast<ASR::Variable_t>(member);
        ASR::ttype_t* member_type = ASRUtils::type_get_past_pointer(
            ASRUtils::type_get_past_allocatable(member_variable->m_type));
        ASR::ttype_t* member_type_ = ASRUtils::type_get_past_array(member_type);
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(member_type, m_dims);
        switch( member_type_->type ) {
            case ASR::ttypeType::StructType: {
                ASR::StructType_t* der = ASR::down_cast<ASR::StructType_t>(member_type_);
                std::string der_type_name = ASRUtils::symbol_name(der->m_derived_type);
                ASR::symbol_t* der_type_sym = current_scope->resolve_symbol(der_type_name);
                if( der_type_sym == nullptr ) {
                    ASR::symbol_t* der_ext;
                    char* module_name = (char*)"~nullptr";
                    ASR::symbol_t* m_external = der->m_derived_type;
                    if( ASR::is_a<ASR::ExternalSymbol_t>(*m_external) ) {
                        ASR::ExternalSymbol_t* m_ext = ASR::down_cast<ASR::ExternalSymbol_t>(m_external);
                        m_external = m_ext->m_external;
                        module_name = m_ext->m_module_name;
                    } else if( ASR::is_a<ASR::Struct_t>(*m_external) ) {
                        ASR::symbol_t* asr_owner = ASRUtils::get_asr_owner(m_external);
                        if( ASR::is_a<ASR::Struct_t>(*asr_owner) ||
                            ASR::is_a<ASR::Module_t>(*asr_owner) ) {
                            module_name = ASRUtils::symbol_name(asr_owner);
                        }
                    }
                    std::string mangled_name = current_scope->get_unique_name(
                                                std::string(module_name) + "_" +
                                                std::string(der_type_name), false);
                    char* mangled_name_char = s2c(al, mangled_name);
                    if( current_scope->get_symbol(mangled_name) == nullptr ) {
                        bool make_new_ext_sym = true;
                        ASR::symbol_t* der_tmp = nullptr;
                        if( current_scope->get_symbol(std::string(der_type_name)) != nullptr ) {
                            der_tmp = current_scope->get_symbol(std::string(der_type_name));
                            if( ASR::is_a<ASR::ExternalSymbol_t>(*der_tmp) ) {
                                ASR::ExternalSymbol_t* der_ext_tmp = ASR::down_cast<ASR::ExternalSymbol_t>(der_tmp);
                                if( der_ext_tmp->m_external == m_external ) {
                                    make_new_ext_sym = false;
                                }
                            } else {
                                make_new_ext_sym = false;
                            }
                        }
                        if( make_new_ext_sym ) {
                            der_ext = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                                            al, loc, current_scope, mangled_name_char, m_external,
                                            module_name, nullptr, 0, s2c(al, der_type_name),
                                            ASR::accessType::Public));
                            current_scope->add_symbol(mangled_name, der_ext);
                        } else {
                            LCOMPILERS_ASSERT(der_tmp != nullptr);
                            der_ext = der_tmp;
                        }
                    } else {
                        der_ext = current_scope->get_symbol(mangled_name);
                    }
                    ASR::asr_t* der_new = ASR::make_StructType_t(al, loc, der_ext);
                    member_type_ = ASRUtils::TYPE(der_new);
                } else if(ASR::is_a<ASR::ExternalSymbol_t>(*der_type_sym)) {
                    member_type_ = ASRUtils::TYPE(ASR::make_StructType_t(al, loc, der_type_sym));
                }
                member_type = ASRUtils::make_Array_t_util(al, loc,
                    member_type_, m_dims, n_dims);
                break;
            }
            default :
                break;
        }

        if( ASR::is_a<ASR::Allocatable_t>(*member_variable->m_type) ) {
            member_type = ASRUtils::TYPE(ASRUtils::make_Allocatable_t_util(al,
            member_variable->base.base.loc, member_type));
        } else if( ASR::is_a<ASR::Pointer_t>(*member_variable->m_type) ) {
            member_type = ASRUtils::TYPE(ASR::make_Pointer_t(al,
            member_variable->base.base.loc, member_type));
        }
        ASR::symbol_t* member_ext = ASRUtils::import_struct_instance_member(al, member, current_scope, member_type);
        ASR::expr_t* value = nullptr;
        v = ASRUtils::symbol_get_past_external(v);
        if (v != nullptr && ASR::down_cast<ASR::Variable_t>(v)->m_storage
                == ASR::storage_typeType::Parameter) {
            if (member_variable->m_symbolic_value != nullptr) {
                value = expr_value(member_variable->m_symbolic_value);
            }
        }
        return ASR::make_StructInstanceMember_t(al, loc, ASRUtils::EXPR(v_var),
            member_ext, ASRUtils::fix_scoped_type(al, member_type, current_scope), value);
    }
}

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
    ASR::binopType op, std::string& intrinsic_op_name,
    SymbolTable* curr_scope, ASR::asr_t*& asr,
    Allocator &al, const Location& loc,
    SetChar& current_function_dependencies,
    SetChar& current_module_dependencies,
    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = ASRUtils::expr_type(right);
    ASR::Struct_t *left_struct = nullptr;
    if ( ASR::is_a<ASR::StructType_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::Struct_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::StructType_t>(
            left_type)->m_derived_type));
    } else if ( ASR::is_a<ASR::ClassType_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::Struct_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::ClassType_t>(
            left_type)->m_class_type));
    }
    bool found = false;
    if( is_op_overloaded(op, intrinsic_op_name, curr_scope, left_struct) ) {
        ASR::symbol_t* sym = curr_scope->resolve_symbol(intrinsic_op_name);
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        if ( left_struct != nullptr && orig_sym == nullptr ) {
            orig_sym = left_struct->m_symtab->resolve_symbol(intrinsic_op_name);
        }
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc;
            if ( ASR::is_a<ASR::ClassProcedure_t>(*gen_proc->m_procs[i]) ) {
                proc =  ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::ClassProcedure_t>(
                    gen_proc->m_procs[i])->m_proc);
            } else {
                proc = gen_proc->m_procs[i];
            }
            if( (void*) proc == (void*) curr_scope->asr_owner ) {
                continue ;
            }
            switch(proc->type) {
                case ASR::symbolType::Function: {
                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
                    std::string matched_func_name = "";
                    if( func->n_args == 2 ) {
                        ASR::ttype_t* left_arg_type = ASRUtils::expr_type(func->m_args[0]);
                        ASR::ttype_t* right_arg_type = ASRUtils::expr_type(func->m_args[1]);
                        if( ASRUtils::check_equal_type(left_arg_type, left_type) &&
                            ASRUtils::check_equal_type(right_arg_type, right_type) ) {
                            found = true;
                            Vec<ASR::call_arg_t> a_args;
                            a_args.reserve(al, 2);
                            ASR::call_arg_t left_call_arg, right_call_arg;
                            left_call_arg.loc = left->base.loc, left_call_arg.m_value = left;
                            a_args.push_back(al, left_call_arg);
                            right_call_arg.loc = right->base.loc, right_call_arg.m_value = right;
                            a_args.push_back(al, right_call_arg);
                            std::string func_name = to_lower(func->m_name);
                            if( curr_scope->resolve_symbol(func_name) ) {
                                matched_func_name = func_name;
                            } else {
                                std::string mangled_name = func_name + "@" + intrinsic_op_name;
                                matched_func_name = mangled_name;
                            }
                            ASR::symbol_t* a_name = curr_scope->resolve_symbol(matched_func_name);
                            if( a_name == nullptr ) {
                                a_name = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                                            al, loc, curr_scope, s2c(al, matched_func_name), proc,
                                            ASRUtils::symbol_name(ASRUtils::get_asr_owner(proc)),
                                            nullptr, 0, func->m_name, ASR::accessType::Public));
                                curr_scope->add_symbol(matched_func_name, a_name);
                            }
                            ASR::ttype_t *return_type = nullptr;
                            if( ASRUtils::get_FunctionType(func)->m_elemental &&
                                func->n_args >= 1 &&
                                ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                ASR::dimension_t* array_dims;
                                size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(
                                ASRUtils::expr_type(a_args[0].m_value), array_dims);
                                Vec<ASR::dimension_t> new_dims;
                                new_dims.from_pointer_n_copy(al, array_dims, array_n_dims);
                                return_type = ASRUtils::duplicate_type(al,
                                                ASRUtils::get_FunctionType(func)->m_return_var_type,
                                                &new_dims);
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                                bool is_array = ASRUtils::is_array(return_type);
                                ASR::dimension_t* m_dims = nullptr;
                                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(return_type, m_dims);
                                return_type = ASRUtils::type_get_past_array(return_type);
                                if( ASR::is_a<ASR::StructType_t>(*return_type) ) {
                                    ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(return_type);
                                    if( curr_scope->get_counter() !=
                                        ASRUtils::symbol_parent_symtab(struct_t->m_derived_type)->get_counter() &&
                                        !curr_scope->resolve_symbol(ASRUtils::symbol_name(struct_t->m_derived_type)) ) {
                                        curr_scope->add_symbol(ASRUtils::symbol_name(struct_t->m_derived_type),
                                            ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al, loc, curr_scope,
                                                ASRUtils::symbol_name(struct_t->m_derived_type), struct_t->m_derived_type,
                                                ASRUtils::symbol_name(ASRUtils::get_asr_owner(struct_t->m_derived_type)), nullptr, 0,
                                                ASRUtils::symbol_name(struct_t->m_derived_type), ASR::accessType::Public)));
                                    }
                                    return_type = ASRUtils::TYPE(ASR::make_StructType_t(al, loc,
                                        curr_scope->resolve_symbol(ASRUtils::symbol_name(struct_t->m_derived_type))));
                                    if( is_array ) {
                                        return_type = ASRUtils::make_Array_t_util(al, loc, return_type, m_dims, n_dims);
                                    }
                                }
                            }
                            if (ASRUtils::symbol_parent_symtab(a_name)->get_counter() != curr_scope->get_counter()) {
                                ADD_ASR_DEPENDENCIES_WITH_NAME(curr_scope, a_name, current_function_dependencies, s2c(al, matched_func_name));
                            }
                            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
                            ASRUtils::set_absent_optional_arguments_to_null(a_args, func, al);
                            asr = ASRUtils::make_FunctionCall_t_util(al, loc, a_name, sym,
                                                            a_args.p, 2,
                                                            return_type,
                                                            nullptr, nullptr,
                                                            false);
                        }
                    }
                    break;
                }
                default: {
                    err("While overloading binary operators only functions can be used",
                                        proc->base.loc);
                }
            }
        }
    }
    return found;
}

void process_overloaded_unary_minus_function(ASR::symbol_t* proc, ASR::expr_t* operand,
    ASR::ttype_t* operand_type, bool& found, Allocator& al, SymbolTable* curr_scope,
    const Location& loc, SetChar& current_function_dependencies,
    SetChar& current_module_dependencies, ASR::asr_t*& asr,
    const std::function<void (const std::string &, const Location &)> /*err*/) {
    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
    std::string matched_func_name = "";
    if( func->n_args == 1 ) {
        ASR::ttype_t* operand_arg_type = ASRUtils::expr_type(func->m_args[0]);
        if( ASRUtils::check_equal_type(operand_arg_type, operand_type) ) {
            found = true;
            Vec<ASR::call_arg_t> a_args;
            a_args.reserve(al, 1);
            ASR::call_arg_t operand_call_arg;
            operand_call_arg.loc = operand->base.loc;
            operand_call_arg.m_value = operand;
            a_args.push_back(al, operand_call_arg);
            std::string func_name = to_lower(func->m_name);
            if( curr_scope->resolve_symbol(func_name) ) {
                matched_func_name = func_name;
            } else {
                std::string mangled_name = func_name + "@~sub";
                matched_func_name = mangled_name;
            }
            ASR::symbol_t* a_name = curr_scope->resolve_symbol(matched_func_name);
            if( a_name == nullptr ) {
                a_name = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                            al, loc, curr_scope, s2c(al, matched_func_name), proc,
                            ASRUtils::symbol_name(ASRUtils::get_asr_owner(proc)),
                            nullptr, 0, func->m_name, ASR::accessType::Public));
                curr_scope->add_symbol(matched_func_name, a_name);
            }
            ASR::ttype_t *return_type = nullptr;
            if( ASRUtils::get_FunctionType(func)->m_elemental &&
                func->n_args >= 1 &&
                ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                ASR::dimension_t* array_dims;
                size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(a_args[0].m_value), array_dims);
                Vec<ASR::dimension_t> new_dims;
                new_dims.from_pointer_n_copy(al, array_dims, array_n_dims);
                return_type = ASRUtils::duplicate_type(al,
                                ASRUtils::get_FunctionType(func)->m_return_var_type,
                                &new_dims);
            } else {
                return_type = ASRUtils::expr_type(func->m_return_var);
                bool is_array = ASRUtils::is_array(return_type);
                ASR::dimension_t* m_dims = nullptr;
                size_t n_dims = ASRUtils::extract_dimensions_from_ttype(return_type, m_dims);
                return_type = ASRUtils::type_get_past_array(return_type);
                if( ASR::is_a<ASR::StructType_t>(*return_type) ) {
                    ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(return_type);
                    if( curr_scope->get_counter() !=
                        ASRUtils::symbol_parent_symtab(struct_t->m_derived_type)->get_counter() &&
                        !curr_scope->resolve_symbol(ASRUtils::symbol_name(struct_t->m_derived_type)) ) {
                        curr_scope->add_symbol(ASRUtils::symbol_name(struct_t->m_derived_type),
                            ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al, loc, curr_scope,
                                ASRUtils::symbol_name(struct_t->m_derived_type), struct_t->m_derived_type,
                                ASRUtils::symbol_name(ASRUtils::get_asr_owner(struct_t->m_derived_type)), nullptr, 0,
                                ASRUtils::symbol_name(struct_t->m_derived_type), ASR::accessType::Public)));
                    }
                    return_type = ASRUtils::TYPE(ASR::make_StructType_t(al, loc,
                        curr_scope->resolve_symbol(ASRUtils::symbol_name(struct_t->m_derived_type))));
                    if( is_array ) {
                        return_type = ASRUtils::make_Array_t_util(
                            al, loc, return_type, m_dims, n_dims);
                    }
                }
            }
            if (ASRUtils::symbol_parent_symtab(a_name)->get_counter() != curr_scope->get_counter()) {
                ADD_ASR_DEPENDENCIES_WITH_NAME(curr_scope, a_name, current_function_dependencies, s2c(al, matched_func_name));
            }
            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
            ASRUtils::set_absent_optional_arguments_to_null(a_args, func, al);
            asr = ASRUtils::make_FunctionCall_t_util(al, loc, a_name, proc,
                                            a_args.p, 1,
                                            return_type,
                                            nullptr, nullptr,
                                            false);
        }
    }
}

bool use_overloaded_unary_minus(ASR::expr_t* operand,
    SymbolTable* curr_scope, ASR::asr_t*& asr,
    Allocator &al, const Location& loc, SetChar& current_function_dependencies,
    SetChar& current_module_dependencies,
    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *operand_type = ASRUtils::expr_type(operand);
    ASR::symbol_t* sym = curr_scope->resolve_symbol("~sub");
    if( !sym ) {
        if( ASR::is_a<ASR::StructType_t>(*operand_type) ) {
            ASR::StructType_t* struct_t = ASR::down_cast<ASR::StructType_t>(operand_type);
            ASR::symbol_t* struct_t_sym = ASRUtils::symbol_get_past_external(struct_t->m_derived_type);
            if( ASR::is_a<ASR::Struct_t>(*struct_t_sym) ) {
                ASR::Struct_t* struct_type_t = ASR::down_cast<ASR::Struct_t>(struct_t_sym);
                sym = struct_type_t->m_symtab->resolve_symbol("~sub");
                while( sym == nullptr && struct_type_t->m_parent != nullptr ) {
                    struct_type_t = ASR::down_cast<ASR::Struct_t>(
                        ASRUtils::symbol_get_past_external(struct_type_t->m_parent));
                    sym = struct_type_t->m_symtab->resolve_symbol("~sub");
                }
                if( sym == nullptr ) {
                    return false;
                }
                sym = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al, loc,
                        curr_scope, s2c(al, "~sub"), sym, struct_type_t->m_name, nullptr, 0,
                        s2c(al, "~sub"), ASR::accessType::Public));
                curr_scope->add_symbol("~sub", sym);
            } else {
                LCOMPILERS_ASSERT(false);
            }
        }
    }

    bool found = false;
    ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
    ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
    for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
        ASR::symbol_t* proc = gen_proc->m_procs[i];
        switch(proc->type) {
            case ASR::symbolType::Function: {
                process_overloaded_unary_minus_function(proc, operand, operand_type,
                    found, al, curr_scope, loc, current_function_dependencies,
                    current_module_dependencies, asr, err);
                break;
            }
            case ASR::symbolType::ClassProcedure: {
                ASR::ClassProcedure_t* class_procedure_t = ASR::down_cast<ASR::ClassProcedure_t>(proc);
                process_overloaded_unary_minus_function(class_procedure_t->m_proc,
                    operand, operand_type, found, al, curr_scope, loc,
                    current_function_dependencies, current_module_dependencies, asr, err);
                break;
            }
            default: {
                err("While overloading binary operators only functions can be used",
                                    proc->base.loc);
            }
        }
    }
    return found;
}

bool is_op_overloaded(ASR::binopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope, ASR::Struct_t* left_struct) {
    bool result = true;
    switch(op) {
        case ASR::binopType::Add: {
            if(intrinsic_op_name != "~add") {
                result = false;
            }
            break;
        }
        case ASR::binopType::Sub: {
            if(intrinsic_op_name != "~sub") {
                result = false;
            }
            break;
        }
        case ASR::binopType::Mul: {
            if(intrinsic_op_name != "~mul") {
                result = false;
            }
            break;
        }
        case ASR::binopType::Div: {
            if(intrinsic_op_name != "~div") {
                result = false;
            }
            break;
        }
        case ASR::binopType::Pow: {
            if(intrinsic_op_name != "~pow") {
                result = false;
            }
            break;
        }
        default: {
            throw LCompilersException("Binary operator '" + ASRUtils::binop_to_str_python(op) + "' not supported yet");
        }
    }

    if( result && curr_scope->resolve_symbol(intrinsic_op_name) == nullptr ) {
        if ( left_struct != nullptr && left_struct->m_symtab->resolve_symbol(
                intrinsic_op_name) != nullptr) {
            result = true;
        } else {
            result = false;
        }
    }
    return result;
}

void process_overloaded_assignment_function(ASR::symbol_t* proc, ASR::expr_t* target, ASR::expr_t* value,
    ASR::ttype_t* target_type, ASR::ttype_t* value_type, bool& found, Allocator& al, const Location& target_loc,
    const Location& value_loc, SymbolTable* curr_scope, SetChar& current_function_dependencies,
    SetChar& current_module_dependencies, ASR::asr_t*& asr, ASR::symbol_t* sym, const Location& loc, ASR::expr_t* expr_dt,
    const std::function<void (const std::string &, const Location &)> err, char* pass_arg=nullptr) {
    ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(proc);
    std::string matched_subrout_name = "";
    if( subrout->n_args == 2 ) {
        ASR::ttype_t* target_arg_type = ASRUtils::expr_type(subrout->m_args[0]);
        ASR::ttype_t* value_arg_type = ASRUtils::expr_type(subrout->m_args[1]);
        if( ASRUtils::types_equal(target_arg_type, target_type) &&
            ASRUtils::types_equal(value_arg_type, value_type) ) {
            std::string arg0_name = ASRUtils::symbol_name(ASR::down_cast<ASR::Var_t>(subrout->m_args[0])->m_v);
            std::string arg1_name = ASRUtils::symbol_name(ASR::down_cast<ASR::Var_t>(subrout->m_args[1])->m_v);
            if( pass_arg != nullptr ) {
                std::string pass_arg_str = std::string(pass_arg);
                if( arg0_name != pass_arg_str && arg1_name != pass_arg_str ) {
                    err(pass_arg_str + " argument is not present in " + std::string(subrout->m_name),
                        proc->base.loc);
                }
                if( (arg0_name == pass_arg_str && target != expr_dt) ) {
                    err(std::string(subrout->m_name) + " is not a procedure of " +
                        ASRUtils::type_to_str_fortran(target_type),
                        loc);
                }
                if( (arg1_name == pass_arg_str && value != expr_dt) ) {
                    err(std::string(subrout->m_name) + " is not a procedure of " +
                        ASRUtils::type_to_str_fortran(value_type),
                        loc);
                }
            }
            found = true;
            Vec<ASR::call_arg_t> a_args;
            a_args.reserve(al, 2);
            ASR::call_arg_t target_arg, value_arg;
            target_arg.loc = target_loc, target_arg.m_value = target;
            a_args.push_back(al, target_arg);
            value_arg.loc = value_loc, value_arg.m_value = value;
            a_args.push_back(al, value_arg);
            std::string subrout_name = to_lower(subrout->m_name);
            if( curr_scope->resolve_symbol(subrout_name) ) {
                matched_subrout_name = subrout_name;
            } else {
                std::string mangled_name = subrout_name + "@~assign";
                matched_subrout_name = mangled_name;
            }
            ASR::symbol_t *a_name = curr_scope->resolve_symbol(matched_subrout_name);
            if( a_name == nullptr ) {
                err("Unable to resolve matched subroutine for assignment overloading, " + matched_subrout_name, loc);
            }
            if (ASRUtils::symbol_parent_symtab(a_name)->get_counter() != curr_scope->get_counter()) {
                ADD_ASR_DEPENDENCIES_WITH_NAME(curr_scope, a_name, current_function_dependencies, s2c(al, matched_subrout_name));
            }
            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
            ASRUtils::set_absent_optional_arguments_to_null(a_args, subrout, al);
            asr = ASRUtils::make_SubroutineCall_t_util(al, loc, a_name, sym,
                                            a_args.p, 2, nullptr, nullptr, false, false);
        }
    }
}

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               SetChar& current_function_dependencies,
                               SetChar& current_module_dependencies,
                               const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *target_type = ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(target));
    ASR::ttype_t *value_type = ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(value));
    bool found = false;
    ASR::symbol_t* sym = curr_scope->resolve_symbol("~assign");
    ASR::expr_t* expr_dt = nullptr;
    if( !sym ) {
        if( ASR::is_a<ASR::StructType_t>(*target_type) ) {
            ASR::Struct_t* target_struct = ASR::down_cast<ASR::Struct_t>(
                ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::StructType_t>(target_type)->m_derived_type));
            sym = target_struct->m_symtab->resolve_symbol("~assign");
            expr_dt = target;
        } else if( ASR::is_a<ASR::StructType_t>(*value_type) ) {
            ASR::Struct_t* value_struct = ASR::down_cast<ASR::Struct_t>(
                ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::StructType_t>(value_type)->m_derived_type));
            sym = value_struct->m_symtab->resolve_symbol("~assign");
            expr_dt = value;
        }
    }
    if (sym) {
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = ASRUtils::symbol_get_past_external(gen_proc->m_procs[i]);
            switch( proc->type ) {
                case ASR::symbolType::Function: {
                    process_overloaded_assignment_function(proc, target, value, target_type,
                        value_type, found, al, target->base.loc, value->base.loc, curr_scope,
                        current_function_dependencies, current_module_dependencies, asr, sym,
                        loc, expr_dt, err);
                    break;
                }
                case ASR::symbolType::ClassProcedure: {
                    ASR::ClassProcedure_t* class_proc = ASR::down_cast<ASR::ClassProcedure_t>(proc);
                    ASR::symbol_t* proc_func = ASR::down_cast<ASR::ClassProcedure_t>(proc)->m_proc;
                    process_overloaded_assignment_function(proc_func, target, value, target_type,
                        value_type, found, al, target->base.loc, value->base.loc, curr_scope,
                        current_function_dependencies, current_module_dependencies, asr, proc_func, loc,
                        expr_dt, err, class_proc->m_self_argument);
                    break;
                }
                default: {
                    err("Only functions and class procedures can be used for generic assignment statement, found " + std::to_string(proc->type), loc);
                }
            }
        }
    }
    return found;
}

void process_overloaded_read_write_function(std::string &read_write, ASR::symbol_t* proc, Vec<ASR::expr_t*> &args,
    ASR::ttype_t* arg_type, bool& found, Allocator& al, const Location& arg_loc,
    SymbolTable* curr_scope, SetChar& current_function_dependencies,
    SetChar& current_module_dependencies, ASR::asr_t*& asr, ASR::symbol_t* sym, const Location& loc, ASR::expr_t* expr_dt,
    const std::function<void (const std::string &, const Location &)> err, char* pass_arg=nullptr) {
    ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(proc);
    std::string matched_subrout_name = "";
    ASR::ttype_t* func_arg_type = ASRUtils::expr_type(subrout->m_args[0]);
    if( ASRUtils::types_equal(func_arg_type, arg_type) ) {
        std::string arg0_name = ASRUtils::symbol_name(ASR::down_cast<ASR::Var_t>(subrout->m_args[0])->m_v);
        if( pass_arg != nullptr ) {
            std::string pass_arg_str = std::string(pass_arg);
            if( (arg0_name == pass_arg_str && args[0] != expr_dt) ) {
                err(std::string(subrout->m_name) + " is not a procedure of " +
                    ASRUtils::type_to_str_fortran(arg_type),
                    loc);
            }
        }
        found = true;
        Vec<ASR::call_arg_t> a_args;
        a_args.reserve(al, args.size());
        for (size_t i = 0; i < args.size(); i++) {
            ASR::call_arg_t arg;
            arg.loc = arg_loc;
            arg.m_value = args[i];
            a_args.push_back(al, arg);
        }
        std::string subrout_name = to_lower(subrout->m_name);
        if( curr_scope->resolve_symbol(subrout_name) ) {
            matched_subrout_name = subrout_name;
        } else {
            std::string mangled_name = subrout_name + "@" + read_write;
            matched_subrout_name = mangled_name;
        }
        ASR::symbol_t *a_name = curr_scope->resolve_symbol(matched_subrout_name);
        if( a_name == nullptr ) {
            err("Unable to resolve matched subroutine for read/write overloading, " + matched_subrout_name, loc);
        }
        if (ASRUtils::symbol_parent_symtab(a_name)->get_counter() != curr_scope->get_counter()) {
            ADD_ASR_DEPENDENCIES_WITH_NAME(curr_scope, a_name, current_function_dependencies, s2c(al, matched_subrout_name));
        }
        ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
        ASRUtils::set_absent_optional_arguments_to_null(a_args, subrout, al);
        asr = ASRUtils::make_SubroutineCall_t_util(al, loc, a_name, sym,
                                        a_args.p, a_args.n, nullptr, nullptr, false, false);
    }
}

bool use_overloaded_file_read_write(std::string &read_write, Vec<ASR::expr_t*> args,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               SetChar& current_function_dependencies,
                               SetChar& current_module_dependencies,
                               const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *arg_type = ASRUtils::type_get_past_allocatable(ASRUtils::expr_type(args[0]));
    bool found = false;
    ASR::symbol_t* sym = curr_scope->resolve_symbol(read_write);
    ASR::expr_t* expr_dt = nullptr;
    if( sym == nullptr ) {
        if( ASR::is_a<ASR::StructType_t>(*arg_type) ) {
            ASR::Struct_t* arg_struct = ASR::down_cast<ASR::Struct_t>(
                ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::StructType_t>(arg_type)->m_derived_type));
            sym = arg_struct->m_symtab->resolve_symbol(read_write);
            expr_dt = args[0];
        }
    } else {
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = ASRUtils::symbol_get_past_external(gen_proc->m_procs[i]);
            switch( proc->type ) {
                case ASR::symbolType::Function: {
                    process_overloaded_read_write_function(read_write, proc, args, arg_type,
                        found, al, args[0]->base.loc, curr_scope,
                        current_function_dependencies, current_module_dependencies, asr, sym,
                        loc, expr_dt, err);
                    break;
                }
                case ASR::symbolType::ClassProcedure: {
                    ASR::ClassProcedure_t* class_proc = ASR::down_cast<ASR::ClassProcedure_t>(proc);
                    ASR::symbol_t* proc_func = ASR::down_cast<ASR::ClassProcedure_t>(proc)->m_proc;
                    process_overloaded_read_write_function(read_write, proc_func, args, arg_type,
                        found, al, args[0]->base.loc, curr_scope,
                        current_function_dependencies, current_module_dependencies, asr, proc_func, loc,
                        expr_dt, err, class_proc->m_self_argument);
                    break;
                }
                default: {
                    err("Only functions and class procedures can be used for generic read/write statement, found " + std::to_string(proc->type), loc);
                }
            }
        }
    }
    return found;
}

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::cmpopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    SetChar& current_function_dependencies,
                    SetChar& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = ASRUtils::expr_type(right);
    ASR::Struct_t *left_struct = nullptr;
    if ( ASR::is_a<ASR::StructType_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::Struct_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::StructType_t>(
            left_type)->m_derived_type));
    } else if ( ASR::is_a<ASR::ClassType_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::Struct_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::ClassType_t>(
            left_type)->m_class_type));
    }
    bool found = false;
    if( is_op_overloaded(op, intrinsic_op_name, curr_scope, left_struct) ) {
        ASR::symbol_t* sym = curr_scope->resolve_symbol(intrinsic_op_name);
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        if ( left_struct != nullptr && orig_sym == nullptr ) {
            orig_sym = left_struct->m_symtab->resolve_symbol(intrinsic_op_name);
        }
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc;
            if ( ASR::is_a<ASR::ClassProcedure_t>(*gen_proc->m_procs[i]) ) {
                proc =  ASRUtils::symbol_get_past_external(
                    ASR::down_cast<ASR::ClassProcedure_t>(
                    gen_proc->m_procs[i])->m_proc);
            } else {
                proc = ASRUtils::symbol_get_past_external(gen_proc->m_procs[i]);
            }
            switch(proc->type) {
                case ASR::symbolType::Function: {
                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
                    std::string matched_func_name = "";
                    if( func->n_args == 2 ) {
                        ASR::ttype_t* left_arg_type = ASRUtils::expr_type(func->m_args[0]);
                        ASR::ttype_t* right_arg_type = ASRUtils::expr_type(func->m_args[1]);
                        if( (left_arg_type->type == left_type->type &&
                            right_arg_type->type == right_type->type)
                        || (ASR::is_a<ASR::ClassType_t>(*left_arg_type) &&
                            ASR::is_a<ASR::StructType_t>(*left_type))
                        || (ASR::is_a<ASR::ClassType_t>(*right_arg_type) &&
                            ASR::is_a<ASR::StructType_t>(*right_type))) {
                            found = true;
                            Vec<ASR::call_arg_t> a_args;
                            a_args.reserve(al, 2);
                            ASR::call_arg_t left_call_arg, right_call_arg;
                            left_call_arg.loc = left->base.loc, left_call_arg.m_value = left;
                            a_args.push_back(al, left_call_arg);
                            right_call_arg.loc = right->base.loc, right_call_arg.m_value = right;
                            a_args.push_back(al, right_call_arg);
                            std::string func_name = to_lower(func->m_name);
                            if( curr_scope->resolve_symbol(func_name) ) {
                                matched_func_name = func_name;
                            } else {
                                std::string mangled_name = func_name + "@" + intrinsic_op_name;
                                matched_func_name = mangled_name;
                            }
                            ASR::symbol_t* a_name = curr_scope->resolve_symbol(matched_func_name);
                            if( a_name == nullptr ) {
                                err("Unable to resolve matched function for operator overloading, " + matched_func_name, loc);
                            }
                            ASR::ttype_t *return_type = nullptr;
                            if( ASRUtils::get_FunctionType(func)->m_elemental &&
                                func->n_args >= 1 &&
                                ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                ASR::dimension_t* array_dims;
                                size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(
                                ASRUtils::expr_type(a_args[0].m_value), array_dims);
                                Vec<ASR::dimension_t> new_dims;
                                new_dims.from_pointer_n_copy(al, array_dims, array_n_dims);
                                return_type = ASRUtils::duplicate_type(al,
                                                ASRUtils::get_FunctionType(func)->m_return_var_type,
                                                &new_dims);
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                            }
                            if (ASRUtils::symbol_parent_symtab(a_name)->get_counter() != curr_scope->get_counter()) {
                                ADD_ASR_DEPENDENCIES_WITH_NAME(curr_scope, a_name, current_function_dependencies, s2c(al, matched_func_name));
                            }
                            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
                            ASRUtils::set_absent_optional_arguments_to_null(a_args, func, al);
                            asr = ASRUtils::make_FunctionCall_t_util(al, loc, a_name, sym,
                                                            a_args.p, 2,
                                                            return_type,
                                                            nullptr, nullptr,
                                                            false);
                        }
                    }
                    break;
                }
                default: {
                    err("While overloading binary operators only functions can be used",
                                        proc->base.loc);
                }
            }
        }
    }
    return found;
}

bool is_op_overloaded(ASR::cmpopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope, ASR::Struct_t *left_struct) {
    bool result = true;
    switch(op) {
        case ASR::cmpopType::Eq: {
            if(intrinsic_op_name != "~eq") {
                result = false;
            }
            break;
        }
        case ASR::cmpopType::NotEq: {
            if(intrinsic_op_name != "~noteq") {
                result = false;
            }
            break;
        }
        case ASR::cmpopType::Lt: {
            if(intrinsic_op_name != "~lt") {
                result = false;
            }
            break;
        }
        case ASR::cmpopType::LtE: {
            if(intrinsic_op_name != "~lte") {
                result = false;
            }
            break;
        }
        case ASR::cmpopType::Gt: {
            if(intrinsic_op_name != "~gt") {
                result = false;
            }
            break;
        }
        case ASR::cmpopType::GtE: {
            if(intrinsic_op_name != "~gte") {
                result = false;
            }
            break;
        }
    }
    if( result && curr_scope->resolve_symbol(intrinsic_op_name) == nullptr ) {
        if ( left_struct != nullptr && left_struct->m_symtab->resolve_symbol(
                intrinsic_op_name) != nullptr) {
            result = true;
        } else {
            result = false;
        }
    }

    return result;
}

template <typename T>
bool argument_types_match(const Vec<ASR::call_arg_t>& args,
        const T &sub) {
    if (args.size() <= sub.n_args) {
        size_t i;
        for (i = 0; i < args.size(); i++) {
            ASR::Variable_t *v = ASRUtils::EXPR2VAR(sub.m_args[i]);
            if (args[i].m_value == nullptr &&
                v->m_presence == ASR::presenceType::Optional) {
                // If it's optional and argument is empty
                // continue to next argument.
                continue;
            }
            // Otherwise this should not be nullptr
            ASR::ttype_t *arg1 = ASRUtils::expr_type(args[i].m_value);
            ASR::ttype_t *arg2 = v->m_type;
            if (!types_equal(arg1, arg2, !ASRUtils::get_FunctionType(sub)->m_elemental)) {
                return false;
            }
        }
        for( ; i < sub.n_args; i++ ) {
            ASR::Variable_t *v = ASRUtils::EXPR2VAR(sub.m_args[i]);
            if( v->m_presence != ASR::presenceType::Optional ) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool select_func_subrout(const ASR::symbol_t* proc, const Vec<ASR::call_arg_t>& args,
                         Location& loc, const std::function<void (const std::string &, const Location &)> err) {
    bool result = false;
    proc = ASRUtils::symbol_get_past_external(proc);
    if (ASR::is_a<ASR::Function_t>(*proc)) {
        ASR::Function_t *fn
            = ASR::down_cast<ASR::Function_t>(proc);
        if (argument_types_match(args, *fn)) {
            result = true;
        }
    } else {
        err("Only Subroutine and Function supported in generic procedure", loc);
    }
    return result;
}

ASR::asr_t* symbol_resolve_external_generic_procedure_without_eval(
            const Location &loc,
            ASR::symbol_t *v, Vec<ASR::call_arg_t>& args,
            SymbolTable* current_scope, Allocator& al,
            const std::function<void (const std::string &, const Location &)> err) {
    ASR::ExternalSymbol_t *p = ASR::down_cast<ASR::ExternalSymbol_t>(v);
    ASR::symbol_t *f2 = ASR::down_cast<ASR::ExternalSymbol_t>(v)->m_external;
    ASR::GenericProcedure_t *g = ASR::down_cast<ASR::GenericProcedure_t>(f2);
    int idx = select_generic_procedure(args, *g, loc, err);
    ASR::symbol_t *final_sym;
    final_sym = g->m_procs[idx];
    LCOMPILERS_ASSERT(ASR::is_a<ASR::Function_t>(*final_sym));
    bool is_subroutine = ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var == nullptr;
    ASR::ttype_t *return_type = nullptr;
    ASR::Function_t* func = nullptr;
    if( ASR::is_a<ASR::Function_t>(*final_sym) ) {
        func = ASR::down_cast<ASR::Function_t>(final_sym);
        if (func->m_return_var) {
            if( ASRUtils::get_FunctionType(func)->m_elemental &&
                func->n_args >= 1 &&
                ASRUtils::is_array(ASRUtils::expr_type(args[0].m_value)) ) {
                ASR::dimension_t* array_dims;
                size_t array_n_dims = ASRUtils::extract_dimensions_from_ttype(
                ASRUtils::expr_type(args[0].m_value), array_dims);
                Vec<ASR::dimension_t> new_dims;
                new_dims.from_pointer_n_copy(al, array_dims, array_n_dims);
                return_type = ASRUtils::duplicate_type(al,
                                ASRUtils::get_FunctionType(func)->m_return_var_type,
                                &new_dims);
            } else {
                return_type = ASRUtils::EXPR2VAR(func->m_return_var)->m_type;
            }
        }
    }
    // Create ExternalSymbol for the final subroutine:
    // We mangle the new ExternalSymbol's local name as:
    //   generic_procedure_local_name @
    //     specific_procedure_remote_name
    std::string local_sym = std::string(p->m_name) + "@"
        + ASRUtils::symbol_name(final_sym);
    if (current_scope->get_symbol(local_sym)
        == nullptr) {
        Str name;
        name.from_str(al, local_sym);
        char *cname = name.c_str(al);
        ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
            al, g->base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ cname,
            final_sym,
            p->m_module_name, nullptr, 0, ASRUtils::symbol_name(final_sym),
            ASR::accessType::Private
            );
        final_sym = ASR::down_cast<ASR::symbol_t>(sub);
        current_scope->add_symbol(local_sym, final_sym);
    } else {
        final_sym = current_scope->get_symbol(local_sym);
    }
    // ASRUtils::insert_module_dependency(v, al, current_module_dependencies);
    if( is_subroutine ) {
        if( func ) {
            ASRUtils::set_absent_optional_arguments_to_null(args, func, al);
        }
        return ASRUtils::make_SubroutineCall_t_util(al, loc, final_sym,
                                        v, args.p, args.size(),
                                        nullptr, nullptr, false, false);
    } else {
        if( func ) {
            ASRUtils::set_absent_optional_arguments_to_null(args, func, al);
        }
        return ASRUtils::make_FunctionCall_t_util(al, loc, final_sym,
                                        v, args.p, args.size(),
                                        return_type,
                                        nullptr, nullptr,
                                        false);
    }
}

ASR::asr_t* make_Cast_t_value(Allocator &al, const Location &a_loc,
            ASR::expr_t* a_arg, ASR::cast_kindType a_kind, ASR::ttype_t* a_type) {

    ASR::expr_t* value = nullptr;

    if (ASRUtils::expr_value(a_arg)) {
        // calculate value
        if (a_kind == ASR::cast_kindType::RealToInteger) {
            int64_t v = ASR::down_cast<ASR::RealConstant_t>(
                        ASRUtils::expr_value(a_arg))->m_r;
            value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_IntegerConstant_t(al, a_loc, v, a_type));
        } else if (a_kind == ASR::cast_kindType::RealToReal) {
            double v = ASR::down_cast<ASR::RealConstant_t>(
                       ASRUtils::expr_value(a_arg))->m_r;
            value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_RealConstant_t(al, a_loc, v, a_type));
        } else if (a_kind == ASR::cast_kindType::RealToComplex) {
            double double_value = ASR::down_cast<ASR::RealConstant_t>(
                                  ASRUtils::expr_value(a_arg))->m_r;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ComplexConstant_t(al, a_loc,
                        double_value, 0, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToReal) {
            // TODO: Clashes with the pow functions
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(al, a_loc, (double)int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToComplex) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ComplexConstant_t(al, a_loc,
                        (double)int_value, 0, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToInteger) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToUnsignedInteger) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_UnsignedIntegerConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::UnsignedIntegerToInteger) {
            int64_t int_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::UnsignedIntegerToUnsignedInteger) {
            int64_t int_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_UnsignedIntegerConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToLogical) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::ComplexToComplex) {
            ASR::ComplexConstant_t* value_complex = ASR::down_cast<ASR::ComplexConstant_t>(
                        ASRUtils::expr_value(a_arg));
            double real = value_complex->m_re;
            double imag = value_complex->m_im;
            value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_ComplexConstant_t(al, a_loc, real, imag, a_type));
        } else if (a_kind == ASR::cast_kindType::ComplexToReal) {
            ASR::ComplexConstant_t* value_complex = ASR::down_cast<ASR::ComplexConstant_t>(
                        ASRUtils::expr_value(a_arg));
            double real = value_complex->m_re;
            value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_RealConstant_t(al, a_loc, real, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToSymbolicExpression) {
            Vec<ASR::expr_t*> args;
            args.reserve(al, 1);
            args.push_back(al, a_arg);
            LCompilers::ASRUtils::create_intrinsic_function create_function =
                LCompilers::ASRUtils::IntrinsicElementalFunctionRegistry::get_create_function("SymbolicInteger");
            diag::Diagnostics diag;
            value = ASR::down_cast<ASR::expr_t>(create_function(al, a_loc, args, diag));
        }
    }

    return ASR::make_Cast_t(al, a_loc, a_arg, a_kind, a_type, value);
}

ASR::symbol_t* import_class_procedure(Allocator &al, const Location& loc,
        ASR::symbol_t* original_sym, SymbolTable *current_scope) {
    if( original_sym && (ASR::is_a<ASR::ClassProcedure_t>(*original_sym) ||
        (ASR::is_a<ASR::Variable_t>(*original_sym) &&
         ASR::is_a<ASR::FunctionType_t>(*ASRUtils::symbol_type(original_sym)))) ) {
        std::string class_proc_name = ASRUtils::symbol_name(original_sym);
        if( original_sym != current_scope->resolve_symbol(class_proc_name) ) {
            std::string imported_proc_name = "1_" + class_proc_name;
            if( current_scope->resolve_symbol(imported_proc_name) == nullptr ) {
                ASR::symbol_t* module_sym = ASRUtils::get_asr_owner(original_sym);
                std::string module_name = ASRUtils::symbol_name(module_sym);
                if( current_scope->resolve_symbol(module_name) == nullptr ) {
                    std::string imported_module_name = "1_" + module_name;
                    if( current_scope->resolve_symbol(imported_module_name) == nullptr ) {
                        LCOMPILERS_ASSERT(ASR::is_a<ASR::Module_t>(
                            *ASRUtils::get_asr_owner(module_sym)));
                        ASR::symbol_t* imported_module = ASR::down_cast<ASR::symbol_t>(
                            ASR::make_ExternalSymbol_t(
                                al, loc, current_scope, s2c(al, imported_module_name),
                                module_sym, ASRUtils::symbol_name(ASRUtils::get_asr_owner(module_sym)),
                                nullptr, 0, s2c(al, module_name), ASR::accessType::Public
                            )
                        );
                        current_scope->add_symbol(imported_module_name, imported_module);
                    }
                    module_name = imported_module_name;
                }
                ASR::symbol_t* imported_sym = ASR::down_cast<ASR::symbol_t>(
                    ASR::make_ExternalSymbol_t(
                        al, loc, current_scope, s2c(al, imported_proc_name),
                        original_sym, s2c(al, module_name), nullptr, 0,
                        ASRUtils::symbol_name(original_sym), ASR::accessType::Public
                    )
                );
                current_scope->add_symbol(imported_proc_name, imported_sym);
                original_sym = imported_sym;
            } else {
                original_sym = current_scope->resolve_symbol(imported_proc_name);
            }
        }
    }
    return original_sym;
}

ASR::asr_t* make_Binop_util(Allocator &al, const Location& loc, ASR::binopType binop,
                        ASR::expr_t* lexpr, ASR::expr_t* rexpr, ASR::ttype_t* ttype) {
    ASRUtils::make_ArrayBroadcast_t_util(al, loc, lexpr, rexpr);
    switch (ttype->type) {
        case ASR::ttypeType::Real: {
            return ASR::make_RealBinOp_t(al, loc, lexpr, binop, rexpr,
                ASRUtils::duplicate_type(al, ttype), nullptr);
        }
        case ASR::ttypeType::Integer: {
            return ASR::make_IntegerBinOp_t(al, loc, lexpr, binop, rexpr,
                ASRUtils::duplicate_type(al, ttype), nullptr);
        }
        case ASR::ttypeType::Complex: {
            return ASR::make_ComplexBinOp_t(al, loc, lexpr, binop, rexpr,
                ASRUtils::duplicate_type(al, ttype), nullptr);
        }
        default:
            throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(ttype));
    }
}

ASR::asr_t* make_Cmpop_util(Allocator &al, const Location& loc, ASR::cmpopType cmpop,
                        ASR::expr_t* lexpr, ASR::expr_t* rexpr, ASR::ttype_t* ttype) {
    ASR::ttype_t* expr_type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
    switch (ttype->type) {
        case ASR::ttypeType::Real: {
            return ASR::make_RealCompare_t(al, loc, lexpr, cmpop, rexpr, expr_type, nullptr);
        }
        case ASR::ttypeType::Integer: {
            return ASR::make_IntegerCompare_t(al, loc, lexpr, cmpop, rexpr, expr_type, nullptr);
        }
        case ASR::ttypeType::Complex: {
            return ASR::make_ComplexCompare_t(al, loc, lexpr, cmpop, rexpr, expr_type, nullptr);
        }
        case ASR::ttypeType::String: {
            return ASR::make_StringCompare_t(al, loc, lexpr, cmpop, rexpr, expr_type, nullptr);
        }
        default:
            throw LCompilersException("Not implemented " + ASRUtils::type_to_str_python(ttype));
    }
}

void make_ArrayBroadcast_t_util(Allocator& al, const Location& loc,
    ASR::expr_t*& expr1, ASR::expr_t*& expr2, ASR::dimension_t* expr1_mdims,
    size_t expr1_ndims) {
    ASR::ttype_t* expr1_type = ASRUtils::expr_type(expr1);
    Vec<ASR::expr_t*> shape_args;
    shape_args.reserve(al, 1);
    shape_args.push_back(al, expr1);
    bool is_value_character_array = ASRUtils::is_character(*ASRUtils::expr_type(expr2));

    Vec<ASR::dimension_t> dims;
    dims.reserve(al, 1);
    ASR::dimension_t dim;
    dim.loc = loc;
    dim.m_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc,
        expr1_ndims, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
    dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc,
        1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
    dims.push_back(al, dim);
    ASR::ttype_t* dest_shape_type = ASRUtils::TYPE(ASR::make_Array_t(al, loc,
        ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4)), dims.p, dims.size(),
        is_value_character_array && !is_value_constant(expr2) ? ASR::array_physical_typeType::StringArraySinglePointer: ASR::array_physical_typeType::FixedSizeArray));

    ASR::expr_t* dest_shape = nullptr;
    ASR::expr_t* value = nullptr;
    ASR::ttype_t* ret_type = nullptr;
    if( ASRUtils::is_fixed_size_array(expr1_mdims, expr1_ndims) ) {
        Vec<ASR::expr_t*> lengths; lengths.reserve(al, expr1_ndims);
        for( size_t i = 0; i < expr1_ndims; i++ ) {
            lengths.push_back(al, ASRUtils::expr_value(expr1_mdims[i].m_length));
        }
        dest_shape = EXPR(ASRUtils::make_ArrayConstructor_t_util(al, loc, lengths.p,
            lengths.size(), dest_shape_type, ASR::arraystorageType::ColMajor));
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t dim;
        dim.loc = loc;
        dim.m_length = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc,
            ASRUtils::get_fixed_size_of_array(expr1_mdims, expr1_ndims),
            ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
        dim.m_start = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc,
            1, ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4))));
        dims.push_back(al, dim);

        if( ASRUtils::is_value_constant(expr2) &&
            ASRUtils::get_fixed_size_of_array(expr1_mdims, expr1_ndims) <= 256 ) {
            ASR::ttype_t* value_type = ASRUtils::TYPE(ASR::make_Array_t(al, loc,
                ASRUtils::type_get_past_array(ASRUtils::expr_type(expr2)), dims.p, dims.size(),
                is_value_character_array && !ASRUtils::is_value_constant(expr2) ? ASR::array_physical_typeType::StringArraySinglePointer: ASR::array_physical_typeType::FixedSizeArray));
            Vec<ASR::expr_t*> values;
            values.reserve(al, ASRUtils::get_fixed_size_of_array(expr1_mdims, expr1_ndims));
            for( int64_t i = 0; i < ASRUtils::get_fixed_size_of_array(expr1_mdims, expr1_ndims); i++ ) {
                values.push_back(al, expr2);
            }
            value = EXPR(ASRUtils::make_ArrayConstructor_t_util(al, loc, values.p,
                values.size(), value_type, ASR::arraystorageType::ColMajor));
            if (ASR::is_a<ASR::ArrayConstructor_t>(*value) && ASRUtils::expr_value(value)) {
                value = ASRUtils::expr_value(value);
            }
            ret_type = value_type;
        }
    } else {
        dest_shape = ASRUtils::EXPR(ASR::make_IntrinsicArrayFunction_t(al, loc,
            static_cast<int64_t>(ASRUtils::IntrinsicArrayFunctions::Shape), shape_args.p,
            shape_args.size(), 0, dest_shape_type, nullptr));
    }

    if (ret_type == nullptr) {
        // TODO: Construct appropriate return type here
        // For now simply coping the type from expr1
        if (ASRUtils::is_simd_array(expr1)) {
            // TODO: Make this more general; do not check for SIMDArray
            ret_type = ASRUtils::duplicate_type(al, expr1_type);
        } else {
            ret_type = expr1_type;
        }
    }
    expr2 = ASRUtils::EXPR(ASR::make_ArrayBroadcast_t(al, loc, expr2, dest_shape, ret_type, value));

    if (ASRUtils::extract_physical_type(expr1_type) != ASRUtils::extract_physical_type(ret_type)) {
        expr2 = ASRUtils::EXPR(ASRUtils::make_ArrayPhysicalCast_t_util(al, loc, expr2,
            ASRUtils::extract_physical_type(ret_type),
            ASRUtils::extract_physical_type(expr1_type), expr1_type, nullptr));
    }
}

void make_ArrayBroadcast_t_util(Allocator& al, const Location& loc,
    ASR::expr_t*& expr1, ASR::expr_t*& expr2) {
    ASR::ttype_t* expr1_type = ASRUtils::expr_type(expr1);
    ASR::ttype_t* expr2_type = ASRUtils::expr_type(expr2);
    ASR::dimension_t *expr1_mdims = nullptr, *expr2_mdims = nullptr;
    size_t expr1_ndims = ASRUtils::extract_dimensions_from_ttype(expr1_type, expr1_mdims);
    size_t expr2_ndims = ASRUtils::extract_dimensions_from_ttype(expr2_type, expr2_mdims);
    if( expr1_ndims == expr2_ndims ) {
        // TODO: Always broadcast both the expressions
        return ;
    }

    if( expr1_ndims > expr2_ndims ) {
        if( ASR::is_a<ASR::ArrayReshape_t>(*expr2) ) {
            return ;
        }
        make_ArrayBroadcast_t_util(al, loc, expr1, expr2, expr1_mdims, expr1_ndims);
    } else {
        if( ASR::is_a<ASR::ArrayReshape_t>(*expr1) ) {
            return ;
        }
        make_ArrayBroadcast_t_util(al, loc, expr2, expr1, expr2_mdims, expr2_ndims);
    }
}

int64_t compute_trailing_zeros(int64_t number, int64_t kind) {
    int64_t trailing_zeros = 0;
    if (number == 0 && kind == 4) {
        return 32;
    } else if (number == 0 && kind == 8) {
        return 64;
    }
    while (number % 2 == 0) {
        number = number / 2;
        trailing_zeros++;
    }
    return trailing_zeros;
}

int64_t compute_leading_zeros(int64_t number, int64_t kind) {
    int64_t leading_zeros = 0;
    int64_t total_bits = 32;
    if (kind == 8) total_bits = 64;
    if (number < 0) return 0;
    while (total_bits > 0) {
        if (number%2 == 0) {
            leading_zeros++;
        } else {
            leading_zeros = 0;
        }
        number = number/2;
        total_bits--;
    }
    return leading_zeros;
}

void append_error(diag::Diagnostics& diag, const std::string& msg,
                const Location& loc) {
    diag.add(diag::Diagnostic(msg, diag::Level::Error,
        diag::Stage::Semantic, {diag::Label("", { loc })}));
}

size_t get_constant_ArrayConstant_size(ASR::ArrayConstant_t* x) {
    return ASRUtils::get_fixed_size_of_array(x->m_type);
}

void get_sliced_indices(ASR::ArraySection_t* arr_sec, std::vector<size_t> &indices) {
    for (size_t i = 0; i < arr_sec->n_args; i++) {
        if (arr_sec->m_args[i].m_step != nullptr) {
            indices.push_back(i + 1);
        }
    }
}

ASR::expr_t* get_ArrayConstant_size(Allocator& al, ASR::ArrayConstant_t* x) {
    ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
    return make_ConstantWithType(make_IntegerConstant_t,
            ASRUtils::get_fixed_size_of_array(x->m_type), int_type, x->base.base.loc);
}

ASR::expr_t* get_ImpliedDoLoop_size(Allocator& al, ASR::ImpliedDoLoop_t* implied_doloop) {
    const Location& loc = implied_doloop->base.base.loc;
    ASRUtils::ASRBuilder builder(al, loc);
    ASR::expr_t* start = implied_doloop->m_start;
    ASR::expr_t* end = implied_doloop->m_end;
    ASR::expr_t* d = implied_doloop->m_increment;
    ASR::expr_t* implied_doloop_size = nullptr;
    int kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(end));
    start = builder.i2i_t(start, ASRUtils::expr_type(end));
    if( d == nullptr ) {
        implied_doloop_size = builder.Add(
            builder.Sub(end, start),
            make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, kind, loc));
    } else {
        implied_doloop_size = builder.Add(builder.Div(
            builder.Sub(end, start), d),
            make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, kind, loc));
    }
    int const_elements = 0;
    ASR::expr_t* implied_doloop_size_ = nullptr;
    for( size_t i = 0; i < implied_doloop->n_values; i++ ) {
        if( ASR::is_a<ASR::ImpliedDoLoop_t>(*implied_doloop->m_values[i]) ) {
            if( implied_doloop_size_ == nullptr ) {
                implied_doloop_size_ = get_ImpliedDoLoop_size(al,
                    ASR::down_cast<ASR::ImpliedDoLoop_t>(implied_doloop->m_values[i]));
            } else {
                implied_doloop_size_ = builder.Add(get_ImpliedDoLoop_size(al,
                    ASR::down_cast<ASR::ImpliedDoLoop_t>(implied_doloop->m_values[i])),
                    implied_doloop_size_);
            }
        } else {
            const_elements += 1;
        }
    }
    if( const_elements > 1 ) {
        if( implied_doloop_size_ == nullptr ) {
            implied_doloop_size_ = make_ConstantWithKind(make_IntegerConstant_t,
                make_Integer_t, const_elements, kind, loc);
        } else {
            implied_doloop_size_ = builder.Add(
                make_ConstantWithKind(make_IntegerConstant_t,
                    make_Integer_t, const_elements, kind, loc),
                implied_doloop_size_);
        }
    }
    if( implied_doloop_size_ ) {
        implied_doloop_size = builder.Mul(implied_doloop_size_, implied_doloop_size);
    }
    return implied_doloop_size;
}

ASR::expr_t* get_ArrayConstructor_size(Allocator& al, ASR::ArrayConstructor_t* x) {
    ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x->base.base.loc, 4));
    ASR::expr_t* array_size = nullptr;
    int64_t constant_size = 0;
    const Location& loc = x->base.base.loc;
    ASRUtils::ASRBuilder builder(al, loc);
    for( size_t i = 0; i < x->n_args; i++ ) {
        ASR::expr_t* element = x->m_args[i];
        if( ASR::is_a<ASR::ArrayConstant_t>(*element) ) {
            if( ASRUtils::is_value_constant(element) ) {
                constant_size += get_constant_ArrayConstant_size(
                    ASR::down_cast<ASR::ArrayConstant_t>(element));
            } else {
                ASR::expr_t* element_array_size = get_ArrayConstant_size(al,
                    ASR::down_cast<ASR::ArrayConstant_t>(element));
                if( array_size == nullptr ) {
                    array_size = element_array_size;
                } else {
                    array_size = builder.Add(array_size,
                                    element_array_size);
                }
            }
        } else if( ASR::is_a<ASR::ArrayConstructor_t>(*element) ) {
            ASR::expr_t* element_array_size = get_ArrayConstructor_size(al,
                ASR::down_cast<ASR::ArrayConstructor_t>(element));
            if( array_size == nullptr ) {
                array_size = element_array_size;
            } else {
                array_size = builder.Add(array_size,
                                element_array_size);
            }
        } else if( ASR::is_a<ASR::Var_t>(*element) ) {
            ASR::ttype_t* element_type = ASRUtils::type_get_past_allocatable(
                ASRUtils::expr_type(element));
            if( ASRUtils::is_array(element_type) ) {
                if( ASRUtils::is_fixed_size_array(element_type) ) {
                    ASR::dimension_t* m_dims = nullptr;
                    size_t n_dims = ASRUtils::extract_dimensions_from_ttype(element_type, m_dims);
                    constant_size += ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
                } else {
                    ASR::expr_t* element_array_size = ASRUtils::get_size(element, al);
                    if( array_size == nullptr ) {
                        array_size = element_array_size;
                    } else {
                        array_size = builder.Add(array_size,
                                        element_array_size);
                    }
                }
            } else {
                constant_size += 1;
            }
        } else if( ASR::is_a<ASR::ImpliedDoLoop_t>(*element) ) {
            ASR::expr_t* implied_doloop_size = get_ImpliedDoLoop_size(al,
                ASR::down_cast<ASR::ImpliedDoLoop_t>(element));
            if( array_size ) {
                array_size = builder.Add(implied_doloop_size, array_size);
            } else {
                array_size = implied_doloop_size;
            }
        } else if( ASR::is_a<ASR::ArraySection_t>(*element) ) {
            ASR::ArraySection_t* array_section_t = ASR::down_cast<ASR::ArraySection_t>(element);
            ASR::expr_t* array_section_size = nullptr;
            for( size_t j = 0; j < array_section_t->n_args; j++ ) {
                ASR::expr_t* start = array_section_t->m_args[j].m_left;
                ASR::expr_t* end = array_section_t->m_args[j].m_right;
                ASR::expr_t* d = array_section_t->m_args[j].m_step;
                if( d == nullptr ) {
                    continue;
                }
                ASR::expr_t* dim_size = builder.Add(builder.Div(
                    builder.Sub(end, start), d),
                    make_ConstantWithKind(make_IntegerConstant_t, make_Integer_t, 1, 4, loc));
                if( array_section_size == nullptr ) {
                    array_section_size = dim_size;
                } else {
                    array_section_size = builder.Mul(array_section_size, dim_size);
                }
            }
            if( array_size == nullptr ) {
                array_size = array_section_size;
            } else {
                builder.Add(array_section_size, array_size);
            }
        } else {
            constant_size += 1;
        }
    }
    ASR::expr_t* constant_size_asr = nullptr;
    if (constant_size == 0 && array_size == nullptr) {
        constant_size = ASRUtils::get_fixed_size_of_array(x->m_type);
    }
    if( constant_size > 0 ) {
        constant_size_asr = make_ConstantWithType(make_IntegerConstant_t,
                                constant_size, int_type, x->base.base.loc);
        if( array_size == nullptr ) {
            return constant_size_asr;
        }
    }
    if( constant_size_asr ) {
        array_size = builder.Add(array_size, constant_size_asr);
    }

    if( array_size == nullptr ) {
        array_size = make_ConstantWithKind(make_IntegerConstant_t,
            make_Integer_t, 0, 4, x->base.base.loc);
    }
    return array_size;
}

ASR::asr_t* make_ArraySize_t_util(
    Allocator &al, const Location &a_loc, ASR::expr_t* a_v,
    ASR::expr_t* a_dim, ASR::ttype_t* a_type, ASR::expr_t* a_value,
    bool for_type) {
    int dim = -1;
    bool is_dimension_constant = (a_dim != nullptr) && ASRUtils::extract_value(
        ASRUtils::expr_value(a_dim), dim);
    ASR::ttype_t* array_func_type = nullptr;
    if( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_v) ) {
        a_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_v)->m_arg;
    }
    if ( ASR::is_a<ASR::IntrinsicArrayFunction_t>(*a_v) && for_type ) {
        ASR::IntrinsicArrayFunction_t* af = ASR::down_cast<ASR::IntrinsicArrayFunction_t>(a_v);
        int64_t dim_index = ASRUtils::IntrinsicArrayFunctionRegistry::get_dim_index(
            static_cast<ASRUtils::IntrinsicArrayFunctions>(af->m_arr_intrinsic_id));
        ASR::expr_t* af_dim = nullptr;
        if( dim_index == 1 && (size_t) dim_index < af->n_args && af->m_args[dim_index] != nullptr ) {
            af_dim = af->m_args[dim_index];
        }
        if ( ASRUtils::is_array(af->m_type) ) {
            array_func_type = af->m_type;
        }
        for ( size_t i = 0; i < af->n_args; i++ ) {
            if ( ASRUtils::is_array(ASRUtils::expr_type(af->m_args[i])) ) {
                a_v = af->m_args[i];
                if ( ASR::is_a<ASR::ArrayPhysicalCast_t>(*a_v)) {
                    a_v = ASR::down_cast<ASR::ArrayPhysicalCast_t>(a_v)->m_arg;
                }
                break;
            }
        }

        if( af_dim != nullptr ) {
            ASRBuilder builder(al, a_loc);
            ASR::ttype_t* int32_type = ASRUtils::TYPE(ASR::make_Integer_t(al, a_loc, 4));
            if( a_dim == nullptr ) {
                size_t rank = ASRUtils::extract_n_dims_from_ttype(ASRUtils::expr_type(a_v));
                Vec<ASR::expr_t*> array_sizes; array_sizes.reserve(al, rank);
                for( size_t i = 1; i <= rank; i++ ) {
                    ASR::expr_t* i_a_dim = ASRUtils::EXPR(
                        ASR::make_IntegerConstant_t(al, a_loc, i, int32_type));
                    ASR::expr_t* i_dim_size = ASRUtils::EXPR(make_ArraySize_t_util(
                        al, a_loc, a_v, i_a_dim, a_type, nullptr, for_type));
                    array_sizes.push_back(al, i_dim_size);
                }

                rank--;
                Vec<ASR::expr_t*> merged_sizes; merged_sizes.reserve(al, rank);
                for( size_t i = 0; i < rank; i++ ) {
                    Vec<ASR::expr_t*> merge_args; merge_args.reserve(al, 3);
                    merge_args.push_back(al, array_sizes[i]);
                    merge_args.push_back(al, array_sizes[i + 1]);
                    merge_args.push_back(al, builder.Lt(builder.i32(i+1), af_dim));
                    diag::Diagnostics diag;
                    merged_sizes.push_back(al, ASRUtils::EXPR(
                        ASRUtils::Merge::create_Merge(al, a_loc, merge_args, diag)));
                }

                ASR::expr_t* size = merged_sizes[0];
                for( size_t i = 1; i < rank; i++ ) {
                    size = builder.Mul(merged_sizes[i], size);
                }

                return &(size->base);
            } else {
                ASR::expr_t *dim_size_lt = ASRUtils::EXPR(make_ArraySize_t_util(
                    al, a_loc, a_v, a_dim, a_type, nullptr, for_type));
                ASR::expr_t *dim_size_gte = ASRUtils::EXPR(make_ArraySize_t_util(
                    al, a_loc, a_v, builder.Add(a_dim, builder.i_t(1, ASRUtils::expr_type(a_dim))),
                    a_type, nullptr, for_type));
                Vec<ASR::expr_t*> merge_args; merge_args.reserve(al, 3);
                merge_args.push_back(al, dim_size_lt); merge_args.push_back(al, dim_size_gte);
                merge_args.push_back(al, builder.Lt(a_dim, af_dim));
                diag::Diagnostics diag;
                return ASRUtils::Merge::create_Merge(al, a_loc, merge_args, diag);
            }
        }
    } else if( ASR::is_a<ASR::FunctionCall_t>(*a_v) && for_type ) {
        ASR::FunctionCall_t* function_call = ASR::down_cast<ASR::FunctionCall_t>(a_v);
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = ASRUtils::extract_dimensions_from_ttype(function_call->m_type, m_dims);
        if( ASRUtils::is_fixed_size_array(function_call->m_type) ) {
            if( a_dim == nullptr ) {
                return ASR::make_IntegerConstant_t(al, a_loc,
                    ASRUtils::get_fixed_size_of_array(function_call->m_type), a_type);
            } else if( is_dimension_constant ) {
                return &(m_dims[dim - 1].m_length->base);
            }
        } else {
            if( a_dim == nullptr ) {
                LCOMPILERS_ASSERT(m_dims[0].m_length);
                ASR::expr_t* result = m_dims[0].m_length;
                for( size_t i = 1; i < n_dims; i++ ) {
                    LCOMPILERS_ASSERT(m_dims[i].m_length);
                    result = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, a_loc,
                        result, ASR::binopType::Mul, m_dims[i].m_length, a_type, nullptr));
                }
                return &(result->base);
            } else if( is_dimension_constant ) {
                LCOMPILERS_ASSERT(m_dims[dim - 1].m_length);
                return &(m_dims[dim - 1].m_length->base);
            }
        }
    } else if( ASR::is_a<ASR::IntrinsicElementalFunction_t>(*a_v) && for_type ) {
        ASR::IntrinsicElementalFunction_t* elemental = ASR::down_cast<ASR::IntrinsicElementalFunction_t>(a_v);
        for( size_t i = 0; i < elemental->n_args; i++ ) {
            if( ASRUtils::is_array(ASRUtils::expr_type(elemental->m_args[i])) ) {
                a_v = elemental->m_args[i];
                break;
            }
        }
    }
    if( ASR::is_a<ASR::ArraySection_t>(*a_v) ) {
        ASR::ArraySection_t* array_section_t = ASR::down_cast<ASR::ArraySection_t>(a_v);
        if( a_dim == nullptr ) {
            ASR::asr_t* const1 = ASR::make_IntegerConstant_t(al, a_loc, 1, a_type);
            ASR::asr_t* size = const1;
            for( size_t i = 0; i < array_section_t->n_args; i++ ) {
                ASR::expr_t* start = array_section_t->m_args[i].m_left;
                ASR::expr_t* end = array_section_t->m_args[i].m_right;
                ASR::expr_t* d = array_section_t->m_args[i].m_step;
                if( (start == nullptr || end == nullptr || d == nullptr) && 
                    !ASRUtils::is_array(ASRUtils::expr_type(end))){
                    continue;
                }
                ASR::expr_t* plus1 = nullptr;
                // Case: A(:, iact) where iact is an array
                if( ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                    ASR::ttype_t* arr_type = ASRUtils::expr_type(end);
                    bool is_func_with_unknown_return = (ASR::is_a<ASR::FunctionCall_t>(*end) &&
                        ASRUtils::is_allocatable(ASRUtils::expr_type(end))) || ASR::is_a<ASR::IntrinsicArrayFunction_t>(*end);  
                    if( ASRUtils::is_fixed_size_array(arr_type) ) {
                        int64_t arr_size = ASRUtils::get_fixed_size_of_array(arr_type);
                        plus1 = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, a_loc, arr_size, a_type));
                    } else {
                        plus1 = ASRUtils::EXPR(ASRUtils::make_ArraySize_t_util(al, end->base.loc, end, 
                            nullptr, a_type, ASRUtils::expr_value(end), !is_func_with_unknown_return));
                    }
                } else {
                    start = CastingUtil::perform_casting(start, a_type, al, a_loc);
                    end = CastingUtil::perform_casting(end, a_type, al, a_loc);
                    d = CastingUtil::perform_casting(d, a_type, al, a_loc);
                    ASR::expr_t* endminusstart = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                        al, a_loc, end, ASR::binopType::Sub, start, a_type, nullptr));
                    ASR::expr_t* byd = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                        al, a_loc, endminusstart, ASR::binopType::Div, d, a_type, nullptr));
                    plus1 = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                        al, a_loc, byd, ASR::binopType::Add, ASRUtils::EXPR(const1), a_type, nullptr));
                }
                size = ASR::make_IntegerBinOp_t(al, a_loc, ASRUtils::EXPR(size),
                    ASR::binopType::Mul, plus1, a_type, nullptr);
            }
            return size;
        } else if( is_dimension_constant ) {
            ASR::asr_t* const1 = ASR::make_IntegerConstant_t(al, a_loc, 1, a_type);
            ASR::expr_t* start = array_section_t->m_args[dim - 1].m_left;
            ASR::expr_t* end = array_section_t->m_args[dim - 1].m_right;
            ASR::expr_t* d = array_section_t->m_args[dim - 1].m_step;

            // Case: A(:, iact) where iact is an array and dim = 2
            if( ASRUtils::is_array(ASRUtils::expr_type(end)) ) {
                bool is_func_with_unknown_return = (ASR::is_a<ASR::FunctionCall_t>(*end) &&
                        ASRUtils::is_allocatable(ASRUtils::expr_type(end))) || ASR::is_a<ASR::IntrinsicArrayFunction_t>(*end);  
                ASR::ttype_t* arr_type = ASRUtils::expr_type(end);
                if( ASRUtils::is_fixed_size_array(arr_type) ) {
                    int64_t arr_size = ASRUtils::get_fixed_size_of_array(arr_type);
                    return ASR::make_IntegerConstant_t(al, a_loc, arr_size, a_type);
                } else {
                    return ASRUtils::make_ArraySize_t_util(al, end->base.loc, end,
                        nullptr, a_type, ASRUtils::expr_value(end), !is_func_with_unknown_return);
                }
            }

            if( start == nullptr && d == nullptr ) {
                return const1;
            }
            start = CastingUtil::perform_casting(start, a_type, al, a_loc);
            end = CastingUtil::perform_casting(end, a_type, al, a_loc);
            d = CastingUtil::perform_casting(d, a_type, al, a_loc);
            ASR::expr_t* endminusstart = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, a_loc, end, ASR::binopType::Sub, start, a_type, nullptr));
            ASR::expr_t* byd = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(
                al, a_loc, endminusstart, ASR::binopType::Div, d, a_type, nullptr));
            return ASR::make_IntegerBinOp_t(al, a_loc, byd, ASR::binopType::Add,
                ASRUtils::EXPR(const1), a_type, nullptr);
        }
    }
    if( ASR::is_a<ASR::ArrayItem_t>(*a_v) ) {
        ASR::ArrayItem_t* array_item_t = ASR::down_cast<ASR::ArrayItem_t>(a_v);
        LCOMPILERS_ASSERT(ASRUtils::is_array(array_item_t->m_type));
        if( for_type ) {
            LCOMPILERS_ASSERT(!ASRUtils::is_allocatable(array_item_t->m_type) &&
                              !ASRUtils::is_pointer(array_item_t->m_type));
        }
        if( a_dim == nullptr ) {
            ASR::asr_t* const1 = ASR::make_IntegerConstant_t(al, a_loc, 1, a_type);
            ASR::asr_t* size = const1;
            for( size_t i = 0; i < array_item_t->n_args; i++ ) {
                ASR::expr_t* end = ASRUtils::EXPR(make_ArraySize_t_util(al, a_loc,
                    array_item_t->m_args[i].m_right, a_dim, a_type, nullptr, for_type));
                size = ASR::make_IntegerBinOp_t(al, a_loc, ASRUtils::EXPR(size),
                    ASR::binopType::Mul, end, a_type, nullptr);
            }
            return size;
        } else if( is_dimension_constant ) {
            return make_ArraySize_t_util(al, a_loc,
                array_item_t->m_args[dim].m_right,
                nullptr, a_type, nullptr, for_type);
        }
    }
    if( is_binop_expr(a_v) && for_type ) {
        if( ASR::is_a<ASR::Var_t>(*extract_member_from_binop(a_v, 1)) ) {
            return make_ArraySize_t_util(al, a_loc, extract_member_from_binop(a_v, 1), a_dim, a_type, a_value, for_type);
        } else {
            return make_ArraySize_t_util(al, a_loc, extract_member_from_binop(a_v, 0), a_dim, a_type, a_value, for_type);
        }
    } else if( is_unaryop_expr(a_v) && for_type ) {
        return make_ArraySize_t_util(al, a_loc, extract_member_from_unaryop(a_v), a_dim, a_type, a_value, for_type);
    } else if( ASR::is_a<ASR::ArrayConstructor_t>(*a_v) && for_type ) {
        ASR::ArrayConstructor_t* array_constructor = ASR::down_cast<ASR::ArrayConstructor_t>(a_v);
        return &(get_ArrayConstructor_size(al, array_constructor)->base);
    } else {
        ASR::dimension_t* m_dims = nullptr;
        size_t n_dims = 0;
        if (array_func_type != nullptr) n_dims = ASRUtils::extract_dimensions_from_ttype(array_func_type, m_dims);
        else n_dims = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(a_v), m_dims);
        bool is_dimension_dependent_only_on_arguments_ = is_dimension_dependent_only_on_arguments(m_dims, n_dims);

        bool compute_size = (is_dimension_dependent_only_on_arguments_ &&
            (is_dimension_constant || a_dim == nullptr));
        if( compute_size && for_type ) {
            ASR::dimension_t* m_dims = nullptr;
            if (array_func_type != nullptr) n_dims = ASRUtils::extract_dimensions_from_ttype(array_func_type, m_dims);
            else n_dims = ASRUtils::extract_dimensions_from_ttype(ASRUtils::expr_type(a_v), m_dims);
            if( a_dim == nullptr ) {
                ASR::asr_t* size = ASR::make_IntegerConstant_t(al, a_loc, 1, a_type);
                for( size_t i = 0; i < n_dims; i++ ) {
                    size = ASR::make_IntegerBinOp_t(al, a_loc, ASRUtils::EXPR(size),
                        ASR::binopType::Mul, m_dims[i].m_length, a_type, nullptr);
                }
                return size;
            } else if( is_dimension_constant ) {
                return (ASR::asr_t*) m_dims[dim - 1].m_length;
            }
        }
    }


    if( for_type ) {
        LCOMPILERS_ASSERT_MSG(
            ASR::is_a<ASR::Var_t>(*a_v) ||
            ASR::is_a<ASR::StructInstanceMember_t>(*a_v) ||
            ASR::is_a<ASR::FunctionParam_t>(*a_v),
            "Found ASR::exprType::" + std::to_string(a_v->type));
    }

    return ASR::make_ArraySize_t(al, a_loc, a_v, a_dim, a_type, a_value);
}

ASR::expr_t* get_compile_time_array_size(Allocator& al, ASR::ttype_t* array_type){
    LCOMPILERS_ASSERT(ASR::is_a<ASR::Array_t>(*
        ASRUtils::type_get_past_allocatable_pointer(array_type)));
    int64_t array_size = ASRUtils::get_fixed_size_of_array(array_type);
    if(array_size != -1){
            return ASRUtils::EXPR(
                ASR::make_IntegerConstant_t(al, array_type->base.loc, array_size,
                ASRUtils::TYPE(ASR::make_Integer_t(al, array_type->base.loc, 8))));
    }
    return nullptr;
}

//Initialize pointer to zero so that it can be initialized in first call to get_instance
ASRUtils::LabelGenerator* ASRUtils::LabelGenerator::label_generator = nullptr;

} // namespace ASRUtils


} // namespace LCompilers
