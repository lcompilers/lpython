#include <lfortran/asr_utils.h>
#include <lfortran/string_utils.h>
#include <lfortran/serialization.h>
#include <lfortran/assert.h>
#include <lfortran/asr_verify.h>
#include <lfortran/utils.h>
#include <lfortran/modfile.h>

namespace LFortran {

    namespace ASRUtils  {


        void visit(int a, std::map<int,std::vector<int>> &deps,
        std::vector<bool> &visited, std::vector<int> &result) {
    visited[a] = true;
    for (auto n : deps[a]) {
        if (!visited[n]) visit(n, deps, visited, result);
    }
    result.push_back(a);
}

std::vector<int> order_deps(std::map<int, std::vector<int>> &deps) {
    std::vector<bool> visited(deps.size(), false);
    std::vector<int> result;
    for (auto d : deps) {
        if (!visited[d.first]) visit(d.first, deps, visited, result);
    }
    return result;
}

std::vector<std::string> order_deps(std::map<std::string, std::vector<std::string>> &deps) {
    // Create a mapping string <-> int
    std::vector<std::string> int2string;
    std::map<std::string, int> string2int;
    for (auto d : deps) {
        if (string2int.find(d.first) == string2int.end()) {
            string2int[d.first] = int2string.size();
            int2string.push_back(d.first);
        }
        for (auto n : d.second) {
            if (string2int.find(n) == string2int.end()) {
                string2int[n] = int2string.size();
                int2string.push_back(n);
            }
        }
    }

    // Transform dep -> dep_int
    std::map<int, std::vector<int>> deps_int;
    for (auto d : deps) {
        deps_int[string2int[d.first]] = std::vector<int>();
        for (auto n : d.second) {
            deps_int[string2int[d.first]].push_back(string2int[n]);
        }
    }

    // Compute ordering
    std::vector<int> result_int = order_deps(deps_int);

    // Transform result_int -> result
    std::vector<std::string> result;
    for (auto n : result_int) {
        result.push_back(int2string[n]);
    }

    return result;
}

std::vector<std::string> determine_module_dependencies(
        const ASR::TranslationUnit_t &unit)
{
    std::map<std::string, std::vector<std::string>> deps;
    for (auto &item : unit.m_global_scope->scope) {
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

ASR::Module_t* extract_module(const ASR::TranslationUnit_t &m) {
    LFORTRAN_ASSERT(m.m_global_scope->scope.size()== 1);
    for (auto &a : m.m_global_scope->scope) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        return ASR::down_cast<ASR::Module_t>(a.second);
    }
    throw LFortranException("ICE: Module not found");
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic) {
    LFORTRAN_ASSERT(symtab);
    if (symtab->scope.find(module_name) != symtab->scope.end()) {
        ASR::symbol_t *m = symtab->scope[module_name];
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            throw SemanticError("The symbol '" + module_name
                + "' is not a module", loc);
        }
    }
    LFORTRAN_ASSERT(symtab->parent == nullptr);
    ASR::TranslationUnit_t *mod1 = find_and_load_module(al, module_name,
            *symtab, intrinsic);
    if (mod1 == nullptr && !intrinsic) {
        // Module not found as a regular module. Try intrinsic module
        if (module_name == "iso_c_binding"
            ||module_name == "iso_fortran_env") {
            mod1 = find_and_load_module(al, "lfortran_intrinsic_" + module_name,
                *symtab, true);
        }
    }
    if (mod1 == nullptr) {
        throw SemanticError("Module '" + module_name + "' not declared in the current source and the modfile was not found",
            loc);
    }
    ASR::Module_t *mod2 = extract_module(*mod1);
    symtab->scope[module_name] = (ASR::symbol_t*)mod2;
    mod2->m_symtab->parent = symtab;
    mod2->m_loaded_from_mod = true;
    LFORTRAN_ASSERT(symtab->resolve_symbol(module_name));

    // Create a temporary TranslationUnit just for fixing the symbols
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
            if (symtab->scope.find(item)
                    == symtab->scope.end()) {
                // A module that was loaded requires to load another
                // module

                // This is not very robust, we should store that information
                // in the ASR itself, or encode in the name in a robust way,
                // such as using `module_name@intrinsic`:
                bool is_intrinsic = startswith(item, "lfortran_intrinsic");
                ASR::TranslationUnit_t *mod1 = find_and_load_module(al,
                        item,
                        *symtab, is_intrinsic);
                if (mod1 == nullptr && !is_intrinsic) {
                    // Module not found as a regular module. Try intrinsic module
                    if (item == "iso_c_binding"
                        ||item == "iso_fortran_env") {
                        mod1 = find_and_load_module(al, "lfortran_intrinsic_" + item,
                            *symtab, true);
                    }
                }

                if (mod1 == nullptr) {
                    throw SemanticError("Module '" + item + "' modfile was not found",
                        loc);
                }
                ASR::Module_t *mod2 = extract_module(*mod1);
                symtab->scope[item] = (ASR::symbol_t*)mod2;
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
        if (symtab->scope.find(item) == symtab->scope.end()) {
            throw SemanticError("ICE: Module '" + item + "' modfile was not found, but should have",
                loc);
        }
    }

    // Fix all external symbols
    fix_external_symbols(*tu, *symtab);
    LFORTRAN_ASSERT(asr_verify(*tu));

    return mod2;
}

void set_intrinsic(ASR::symbol_t* sym) {
    switch( sym->type ) {
        case ASR::symbolType::Module: {
            ASR::Module_t* module_sym = ASR::down_cast<ASR::Module_t>(sym);
            for( auto& itr: module_sym->m_symtab->scope ) {
                set_intrinsic(itr.second);
            }
            break;
        }
        case ASR::symbolType::Function: {
            ASR::Function_t* function_sym = ASR::down_cast<ASR::Function_t>(sym);
            function_sym->m_abi = ASR::abiType::Intrinsic;
            break;
        }
        case ASR::symbolType::Subroutine: {
            ASR::Subroutine_t* subroutine_sym = ASR::down_cast<ASR::Subroutine_t>(sym);
            subroutine_sym->m_abi = ASR::abiType::Intrinsic;
            break;
        }
        case ASR::symbolType::DerivedType: {
            ASR::DerivedType_t* derived_type_sym = ASR::down_cast<ASR::DerivedType_t>(sym);
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
    for( auto& itr: trans_unit->m_global_scope->scope ) {
        set_intrinsic(itr.second);
    }
}

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic) {
    std::string modfilename = msym + ".mod";
    if (intrinsic) {
        std::string rl_path = get_runtime_library_dir();
        modfilename = rl_path + "/" + modfilename;
    }
    std::string modfile = read_file(modfilename);
    if (modfile == "") return nullptr;
    ASR::TranslationUnit_t *asr = load_modfile(al, modfile, false,
        symtab);
    if (intrinsic) {
        set_intrinsic(asr);
    }
    return asr;
}
    } // namespace ASRUtils


} // namespace LFortran
