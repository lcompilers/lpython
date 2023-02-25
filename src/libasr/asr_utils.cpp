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
    for (auto &item : unit.m_global_scope->get_scope()) {
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
    for (auto &a : m.m_global_scope->get_scope()) {
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

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m) {
    LCOMPILERS_ASSERT(m.m_global_scope->get_scope().size()== 1);
    for (auto &a : m.m_global_scope->get_scope()) {
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
                            const std::function<void (const std::string &, const Location &)> err) {
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
            *symtab, intrinsic, pass_options);
    if (mod1 == nullptr && !intrinsic) {
        // Module not found as a regular module. Try intrinsic module
        if (module_name == "iso_c_binding"
            ||module_name == "iso_fortran_env"
            ||module_name == "ieee_arithmetic") {
            mod1 = find_and_load_module(al, "lfortran_intrinsic_" + module_name,
                *symtab, true, pass_options);
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
                        *symtab, is_intrinsic, pass_options);
                if (mod1 == nullptr && !is_intrinsic) {
                    // Module not found as a regular module. Try intrinsic module
                    if (item == "iso_c_binding"
                        ||item == "iso_fortran_env") {
                        mod1 = find_and_load_module(al, "lfortran_intrinsic_" + item,
                            *symtab, true, pass_options);
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
        case ASR::symbolType::StructType: {
            ASR::StructType_t* derived_type_sym = ASR::down_cast<ASR::StructType_t>(sym);
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
    for( auto& itr: trans_unit->m_global_scope->get_scope() ) {
        set_intrinsic(itr.second);
    }
}

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                LCompilers::PassOptions& pass_options) {
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
            ASR::TranslationUnit_t *asr = load_modfile(al, modfile, false, symtab);
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
    if (ASR::is_a<ASR::StructType_t>(*member)) {
        ASR::StructType_t* member_variable = ASR::down_cast<ASR::StructType_t>(member);
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
        ASR::ttype_t* member_type = ASRUtils::TYPE(ASR::make_Struct_t(al,
            member_variable->base.base.loc, mem_es, nullptr, 0));
        return ASR::make_StructInstanceMember_t(al, loc, ASRUtils::EXPR(v_var),
                mem_es, member_type, nullptr);
    } else {
        LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member));
        ASR::Variable_t* member_variable = ASR::down_cast<ASR::Variable_t>(member);
        ASR::ttype_t* member_type = member_variable->m_type;
        bool is_pointer = false;
        if (ASRUtils::is_pointer(member_type)) {
            is_pointer = true;
            member_type = ASR::down_cast<ASR::Pointer_t>(member_type)->m_type;
        }
        switch( member_type->type ) {
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* der = ASR::down_cast<ASR::Struct_t>(member_type);
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
                    } else if( ASR::is_a<ASR::StructType_t>(*m_external) ) {
                        ASR::symbol_t* asr_owner = ASRUtils::get_asr_owner(m_external);
                        if( ASR::is_a<ASR::StructType_t>(*asr_owner) ||
                            ASR::is_a<ASR::Module_t>(*asr_owner) ) {
                            module_name = ASRUtils::symbol_name(asr_owner);
                        }
                    }
                    std::string mangled_name = current_scope->get_unique_name(
                                                std::string(module_name) + "_" +
                                                std::string(der_type_name));
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
                    ASR::asr_t* der_new = ASR::make_Struct_t(al, loc, der_ext, der->m_dims, der->n_dims);
                    member_type = ASRUtils::TYPE(der_new);
                } else if(ASR::is_a<ASR::ExternalSymbol_t>(*der_type_sym)) {
                    member_type = ASRUtils::TYPE(ASR::make_Struct_t(al, loc, der_type_sym,
                                    der->m_dims, der->n_dims));
                }
                break;
            }
            default :
                break;
        }
        if (is_pointer) {
            member_type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, member_type));
        }
        ASR::ttype_t* member_type_ = nullptr;
        ASR::symbol_t* member_ext = ASRUtils::import_struct_instance_member(al, member, current_scope, member_type_);
        ASR::expr_t* value = nullptr;
        v = ASRUtils::symbol_get_past_external(v);
        if (v != nullptr && ASR::down_cast<ASR::Variable_t>(v)->m_storage
                == ASR::storage_typeType::Parameter) {
            if (member_variable->m_symbolic_value != nullptr) {
                value = expr_value(member_variable->m_symbolic_value);
            }
        }
        return ASR::make_StructInstanceMember_t(al, loc, ASRUtils::EXPR(v_var),
            member_ext, member_type, value);
    }
}

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::binopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc,
                    std::set<std::string>& current_function_dependencies,
                    Vec<char*>& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = ASRUtils::expr_type(right);
    bool found = false;
    if( is_op_overloaded(op, intrinsic_op_name, curr_scope) ) {
        ASR::symbol_t* sym = curr_scope->resolve_symbol(intrinsic_op_name);
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = gen_proc->m_procs[i];
            switch(proc->type) {
                case ASR::symbolType::Function: {
                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
                    std::string matched_func_name = "";
                    if( func->n_args == 2 ) {
                        ASR::ttype_t* left_arg_type = ASRUtils::expr_type(func->m_args[0]);
                        ASR::ttype_t* right_arg_type = ASRUtils::expr_type(func->m_args[1]);
                        if( left_arg_type->type == left_type->type &&
                            right_arg_type->type == right_type->type ) {
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
                                func->n_args == 1 &&
                                ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(a_args[0].m_value));
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                            }
                            current_function_dependencies.insert(matched_func_name);
                            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
                            asr = ASR::make_FunctionCall_t(al, loc, a_name, sym,
                                                            a_args.p, 2,
                                                            return_type,
                                                            nullptr, nullptr);
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

bool is_op_overloaded(ASR::binopType op, std::string& intrinsic_op_name,
                      SymbolTable* curr_scope) {
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
    if( result && curr_scope->get_symbol(intrinsic_op_name) == nullptr ) {
        result = false;
    }
    return result;
}

void process_overloaded_assignment_function(ASR::symbol_t* proc, ASR::expr_t* target, ASR::expr_t* value,
    ASR::ttype_t* target_type, ASR::ttype_t* value_type, bool& found, Allocator& al, const Location& target_loc,
    const Location& value_loc, SymbolTable* curr_scope, std::set<std::string>& current_function_dependencies,
    Vec<char*>& current_module_dependencies, ASR::asr_t*& asr, ASR::symbol_t* sym, const Location& loc, ASR::expr_t* expr_dt,
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
                        ASRUtils::type_to_str(target_type),
                        loc);
                }
                if( (arg1_name == pass_arg_str && value != expr_dt) ) {
                    err(std::string(subrout->m_name) + " is not a procedure of " +
                        ASRUtils::type_to_str(value_type),
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
            current_function_dependencies.insert(matched_subrout_name);
            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
            asr = ASR::make_SubroutineCall_t(al, loc, a_name, sym,
                                            a_args.p, 2, nullptr);
        }
    }
}

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               std::set<std::string>& current_function_dependencies,
                               Vec<char*>& current_module_dependencies,
                               const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *target_type = ASRUtils::expr_type(target);
    ASR::ttype_t *value_type = ASRUtils::expr_type(value);
    bool found = false;
    ASR::symbol_t* sym = curr_scope->resolve_symbol("~assign");
    ASR::expr_t* expr_dt = nullptr;
    if( !sym ) {
        if( ASR::is_a<ASR::Struct_t>(*target_type) ) {
            ASR::StructType_t* target_struct = ASR::down_cast<ASR::StructType_t>(
                ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Struct_t>(target_type)->m_derived_type));
            sym = target_struct->m_symtab->resolve_symbol("~assign");
            expr_dt = target;
        } else if( ASR::is_a<ASR::Struct_t>(*value_type) ) {
            ASR::StructType_t* value_struct = ASR::down_cast<ASR::StructType_t>(
                ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Struct_t>(value_type)->m_derived_type));
            sym = value_struct->m_symtab->resolve_symbol("~assign");
            expr_dt = value;
        }
    }
    if (sym) {
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = gen_proc->m_procs[i];
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
                    err("Only functions and class procedures can be used for generic assignment statement", loc);
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
                    std::set<std::string>& current_function_dependencies,
                    Vec<char*>& current_module_dependencies,
                    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = ASRUtils::expr_type(right);
    ASR::StructType_t *left_struct = nullptr;
    if ( ASR::is_a<ASR::Struct_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::StructType_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Struct_t>(
            left_type)->m_derived_type));
    } else if ( ASR::is_a<ASR::Class_t>(*left_type) ) {
        left_struct = ASR::down_cast<ASR::StructType_t>(
            ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Class_t>(
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
            switch(proc->type) {
                case ASR::symbolType::Function: {
                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
                    std::string matched_func_name = "";
                    if( func->n_args == 2 ) {
                        ASR::ttype_t* left_arg_type = ASRUtils::expr_type(func->m_args[0]);
                        ASR::ttype_t* right_arg_type = ASRUtils::expr_type(func->m_args[1]);
                        if( (left_arg_type->type == left_type->type &&
                            right_arg_type->type == right_type->type)
                        || (ASR::is_a<ASR::Class_t>(*left_arg_type) &&
                            ASR::is_a<ASR::Struct_t>(*left_type))
                        || (ASR::is_a<ASR::Class_t>(*right_arg_type) &&
                            ASR::is_a<ASR::Struct_t>(*right_type))) {
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
                                func->n_args == 1 &&
                                ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(a_args[0].m_value));
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                            }
                            current_function_dependencies.insert(matched_func_name);
                            ASRUtils::insert_module_dependency(a_name, al, current_module_dependencies);
                            asr = ASR::make_FunctionCall_t(al, loc, a_name, sym,
                                                            a_args.p, 2,
                                                            return_type,
                                                            nullptr, nullptr);
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
                      SymbolTable* curr_scope, ASR::StructType_t *left_struct) {
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
            if (!types_equal(arg1, arg2)) {
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

int select_generic_procedure(const Vec<ASR::call_arg_t>& args,
        const ASR::GenericProcedure_t &p, Location loc,
        const std::function<void (const std::string &, const Location &)> err,
        bool raise_error) {
    for (size_t i=0; i < p.n_procs; i++) {
        if( ASR::is_a<ASR::ClassProcedure_t>(*p.m_procs[i]) ) {
            ASR::ClassProcedure_t *clss_fn
                = ASR::down_cast<ASR::ClassProcedure_t>(p.m_procs[i]);
            const ASR::symbol_t *proc = ASRUtils::symbol_get_past_external(clss_fn->m_proc);
            if( select_func_subrout(proc, args, loc, err) ) {
                return i;
            }
        } else {
            if( select_func_subrout(p.m_procs[i], args, loc, err) ) {
                return i;
            }
        }
    }
    if( raise_error ) {
        err("Arguments do not match for any generic procedure, " + std::string(p.m_name), loc);
    }
    return -1;
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
    if( ASR::is_a<ASR::Function_t>(*final_sym) ) {
        ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(final_sym);
        if (func->m_return_var) {
            if( ASRUtils::get_FunctionType(func)->m_elemental &&
                func->n_args == 1 &&
                ASRUtils::is_array(ASRUtils::expr_type(args[0].m_value)) ) {
                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(args[0].m_value));
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
        return ASR::make_SubroutineCall_t(al, loc, final_sym,
                                        v, args.p, args.size(),
                                        nullptr);
    } else {
        return ASR::make_FunctionCall_t(al, loc, final_sym,
                                        v, args.p, args.size(),
                                        return_type,
                                        nullptr, nullptr);
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
            // int64_t value = ASR::down_cast<ASR::ConstantInteger_t>(ASRUtils::expr_value(a_arg))->m_n;
            // value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(al, a_loc, (double)v, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToComplex) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ComplexConstant_t(al, a_loc,
                        (double)int_value, 0, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToInteger) {
            int64_t int_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                ASRUtils::expr_value(a_arg))->m_n;
            value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, a_loc, int_value, a_type));
        } else if (a_kind == ASR::cast_kindType::IntegerToLogical) {
            // TODO: implement
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
        }
    }

    return ASR::make_Cast_t(al, a_loc, a_arg, a_kind, a_type, value);
}

ASR::symbol_t* import_class_procedure(Allocator &al, const Location& loc,
        ASR::symbol_t* original_sym, SymbolTable *current_scope) {
    if( original_sym && ASR::is_a<ASR::ClassProcedure_t>(*original_sym) ) {
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

//Initialize pointer to zero so that it can be initialized in first call to get_instance
ASRUtils::LabelGenerator* ASRUtils::LabelGenerator::label_generator = nullptr;

} // namespace ASRUtils


} // namespace LCompilers
