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
    symtab->asr_owner = orig_asr_owner;

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

ASR::asr_t* getDerivedRef_t(Allocator& al, const Location& loc,
                            ASR::asr_t* v_var, ASR::symbol_t* member,
                            SymbolTable* current_scope) {
    ASR::Variable_t* member_variable = ((ASR::Variable_t*)(&(member->base)));
    ASR::ttype_t* member_type = member_variable->m_type;
    switch( member_type->type ) {
        case ASR::ttypeType::Derived: {
            ASR::Derived_t* der = (ASR::Derived_t*)(&(member_type->base));
            ASR::DerivedType_t* der_type = (ASR::DerivedType_t*)(&(der->m_derived_type->base));
            if( der_type->m_symtab->counter != current_scope->counter ) {
                ASR::symbol_t* der_ext;
                char* module_name = (char*)"~nullptr";
                ASR::symbol_t* m_external = der->m_derived_type;
                if( m_external->type == ASR::symbolType::ExternalSymbol ) {
                    ASR::ExternalSymbol_t* m_ext = (ASR::ExternalSymbol_t*)(&(m_external->base));
                    m_external = m_ext->m_external;
                    module_name = m_ext->m_module_name;
                }
                Str mangled_name;
                mangled_name.from_str(al, "1_" +
                                            std::string(module_name) + "_" +
                                            std::string(der_type->m_name));
                char* mangled_name_char = mangled_name.c_str(al);
                if( current_scope->scope.find(mangled_name.str()) == current_scope->scope.end() ) {
                    bool make_new_ext_sym = true;
                    ASR::symbol_t* der_tmp = nullptr;
                    if( current_scope->scope.find(std::string(der_type->m_name)) != current_scope->scope.end() ) {
                        der_tmp = current_scope->scope[std::string(der_type->m_name)];
                        if( der_tmp->type == ASR::symbolType::ExternalSymbol ) {
                            ASR::ExternalSymbol_t* der_ext_tmp = (ASR::ExternalSymbol_t*)(&(der_tmp->base));
                            if( der_ext_tmp->m_external == m_external ) {
                                make_new_ext_sym = false;
                            }
                        } else {
                            make_new_ext_sym = false;
                        }
                    }
                    if( make_new_ext_sym ) {
                        der_ext = (ASR::symbol_t*)ASR::make_ExternalSymbol_t(al, loc, current_scope, mangled_name_char, m_external,
                                                                            module_name, nullptr, 0, der_type->m_name, ASR::accessType::Public);
                        current_scope->scope[mangled_name.str()] = der_ext;
                    } else {
                        LFORTRAN_ASSERT(der_tmp != nullptr);
                        der_ext = der_tmp;
                    }
                } else {
                    der_ext = current_scope->scope[mangled_name.str()];
                }
                ASR::asr_t* der_new = ASR::make_Derived_t(al, loc, der_ext, der->m_dims, der->n_dims);
                member_type = (ASR::ttype_t*)(der_new);
            }
            break;
        }
        default :
            break;
    }
    return ASR::make_DerivedRef_t(al, loc, LFortran::ASRUtils::EXPR(v_var), member, member_type, nullptr);
}

bool use_overloaded(ASR::expr_t* left, ASR::expr_t* right,
                    ASR::binopType op, std::string& intrinsic_op_name,
                    SymbolTable* curr_scope, ASR::asr_t*& asr,
                    Allocator &al, const Location& loc) {
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
    bool found = false;
    if( is_op_overloaded(op, intrinsic_op_name, curr_scope) ) {
        ASR::symbol_t* sym = curr_scope->scope[intrinsic_op_name];
        const ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = gen_proc->m_procs[i];
            switch(proc->type) {
                case ASR::symbolType::Function: {
                    ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(proc);
                    if( func->n_args == 2 ) {
                        ASR::ttype_t* left_arg_type = ASRUtils::expr_type(func->m_args[0]);
                        ASR::ttype_t* right_arg_type = ASRUtils::expr_type(func->m_args[1]);
                        if( left_arg_type->type == left_type->type && 
                            right_arg_type->type == right_type->type ) {
                            found = true;
                            Vec<ASR::expr_t*> a_args;
                            a_args.reserve(al, 2);
                            a_args.push_back(al, left);
                            a_args.push_back(al, right);
                            asr = ASR::make_FunctionCall_t(al, loc, curr_scope->scope[std::string(func->m_name)], nullptr,
                                                            a_args.p, 2, nullptr, 0,
                                                            ASRUtils::expr_type(func->m_return_var),
                                                            nullptr, nullptr);
                        }
                    }
                    break;
                }
                default: {
                    throw SemanticError("While overloading binary operators only functions can be used",
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
    }
    if( result && curr_scope->scope.find(intrinsic_op_name) == curr_scope->scope.end() ) {
        result = false;
    }
    return result;
}
    } // namespace ASRUtils


} // namespace LFortran
