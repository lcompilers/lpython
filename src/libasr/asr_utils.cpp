#include <unordered_set>
#include <map>
#include <libasr/asr_utils.h>
#include <libasr/string_utils.h>
#include <libasr/serialization.h>
#include <libasr/assert.h>
#include <libasr/asr_verify.h>
#include <libasr/utils.h>
#include <libasr/modfile.h>

namespace LFortran {

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
    LFORTRAN_ASSERT(m.m_global_scope->get_scope().size()== 1);
    for (auto &a : m.m_global_scope->get_scope()) {
        LFORTRAN_ASSERT(ASR::is_a<ASR::Module_t>(*a.second));
        return ASR::down_cast<ASR::Module_t>(a.second);
    }
    throw LCompilersException("ICE: Module not found");
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            const std::string &rl_path,
                            bool run_verify,
                            const std::function<void (const std::string &, const Location &)> err) {
    LFORTRAN_ASSERT(symtab);
    if (symtab->get_symbol(module_name) != nullptr) {
        ASR::symbol_t *m = symtab->get_symbol(module_name);
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            err("The symbol '" + module_name + "' is not a module", loc);
        }
    }
    LFORTRAN_ASSERT(symtab->parent == nullptr);
    ASR::TranslationUnit_t *mod1 = find_and_load_module(al, module_name,
            *symtab, intrinsic, rl_path);
    if (mod1 == nullptr && !intrinsic) {
        // Module not found as a regular module. Try intrinsic module
        if (module_name == "iso_c_binding"
            ||module_name == "iso_fortran_env"
            ||module_name == "ieee_arithmetic") {
            mod1 = find_and_load_module(al, "lfortran_intrinsic_" + module_name,
                *symtab, true, rl_path);
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
                        *symtab, is_intrinsic, rl_path);
                if (mod1 == nullptr && !is_intrinsic) {
                    // Module not found as a regular module. Try intrinsic module
                    if (item == "iso_c_binding"
                        ||item == "iso_fortran_env") {
                        mod1 = find_and_load_module(al, "lfortran_intrinsic_" + item,
                            *symtab, true, rl_path);
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
    if (run_verify) {
        LFORTRAN_ASSERT(asr_verify(*tu));
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
            function_sym->m_abi = ASR::abiType::Intrinsic;
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
    for( auto& itr: trans_unit->m_global_scope->get_scope() ) {
        set_intrinsic(itr.second);
    }
}

ASR::TranslationUnit_t* find_and_load_module(Allocator &al, const std::string &msym,
                                                SymbolTable &symtab, bool intrinsic,
                                                const std::string &rl_path) {
    std::string modfilename = msym + ".mod";
    if (intrinsic) {
        modfilename = rl_path + "/" + modfilename;
    }

    std::string modfile;
    if (!read_file(modfilename, modfile)) return nullptr;
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
            ASR::Derived_t* der = ASR::down_cast<ASR::Derived_t>(member_type);
            std::string der_type_name = ASRUtils::symbol_name(der->m_derived_type);
            ASR::symbol_t* der_type_sym = current_scope->resolve_symbol(der_type_name);
            if( der_type_sym == nullptr ) {
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
                                            std::string(der_type_name));
                char* mangled_name_char = mangled_name.c_str(al);
                if( current_scope->get_symbol(mangled_name.str()) == nullptr ) {
                    bool make_new_ext_sym = true;
                    ASR::symbol_t* der_tmp = nullptr;
                    if( current_scope->get_symbol(std::string(der_type_name)) != nullptr ) {
                        der_tmp = current_scope->get_symbol(std::string(der_type_name));
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
                                                                            module_name, nullptr, 0, s2c(al, der_type_name), ASR::accessType::Public);
                        current_scope->add_symbol(mangled_name.str(), der_ext);
                    } else {
                        LFORTRAN_ASSERT(der_tmp != nullptr);
                        der_ext = der_tmp;
                    }
                } else {
                    der_ext = current_scope->get_symbol(mangled_name.str());
                }
                ASR::asr_t* der_new = ASR::make_Derived_t(al, loc, der_ext, der->m_dims, der->n_dims);
                member_type = ASRUtils::TYPE(der_new);
            } else if(ASR::is_a<ASR::ExternalSymbol_t>(*der_type_sym)) {
                member_type = ASRUtils::TYPE(ASR::make_Derived_t(al, loc, der_type_sym,
                                der->m_dims, der->n_dims));
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
                    Allocator &al, const Location& loc,
                    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
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
                            if( func->m_elemental && func->n_args == 1 && ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(a_args[0].m_value));
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                            }
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

bool use_overloaded_assignment(ASR::expr_t* target, ASR::expr_t* value,
                               SymbolTable* curr_scope, ASR::asr_t*& asr,
                               Allocator &al, const Location& loc,
                               const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *target_type = LFortran::ASRUtils::expr_type(target);
    ASR::ttype_t *value_type = LFortran::ASRUtils::expr_type(value);
    bool found = false;
    ASR::symbol_t* sym = curr_scope->resolve_symbol("~assign");
    if (sym) {
        ASR::symbol_t* orig_sym = ASRUtils::symbol_get_past_external(sym);
        ASR::CustomOperator_t* gen_proc = ASR::down_cast<ASR::CustomOperator_t>(orig_sym);
        for( size_t i = 0; i < gen_proc->n_procs && !found; i++ ) {
            ASR::symbol_t* proc = gen_proc->m_procs[i];
            ASR::Function_t* subrout = ASR::down_cast<ASR::Function_t>(proc);
            std::string matched_subrout_name = "";
            if( subrout->n_args == 2 ) {
                ASR::ttype_t* target_arg_type = ASRUtils::expr_type(subrout->m_args[0]);
                ASR::ttype_t* value_arg_type = ASRUtils::expr_type(subrout->m_args[1]);
                if( target_arg_type->type == target_type->type &&
                    value_arg_type->type == value_type->type ) {
                    found = true;
                    Vec<ASR::call_arg_t> a_args;
                    a_args.reserve(al, 2);
                    ASR::call_arg_t target_arg, value_arg;
                    target_arg.loc = target->base.loc, target_arg.m_value = target;
                    a_args.push_back(al, target_arg);
                    value_arg.loc = value->base.loc, value_arg.m_value = value;
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
                    asr = ASR::make_SubroutineCall_t(al, loc, a_name, sym,
                                                     a_args.p, 2, nullptr);
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
                    const std::function<void (const std::string &, const Location &)> err) {
    ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
    ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
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
                            if( func->m_elemental && func->n_args == 1 && ASRUtils::is_array(ASRUtils::expr_type(a_args[0].m_value)) ) {
                                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(a_args[0].m_value));
                            } else {
                                return_type = ASRUtils::expr_type(func->m_return_var);
                            }
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
                      SymbolTable* curr_scope) {
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
        result = false;
    }
    return result;
}

bool is_parent(ASR::DerivedType_t* a, ASR::DerivedType_t* b) {
    ASR::symbol_t* current_parent = b->m_parent;
    while( current_parent ) {
        if( current_parent == (ASR::symbol_t*) a ) {
            return true;
        }
        LFORTRAN_ASSERT(ASR::is_a<ASR::DerivedType_t>(*current_parent));
        current_parent = ASR::down_cast<ASR::DerivedType_t>(current_parent)->m_parent;
    }
    return false;
}

bool is_derived_type_similar(ASR::DerivedType_t* a, ASR::DerivedType_t* b) {
    return a == b || is_parent(a, b) || is_parent(b, a);
}

bool types_equal(const ASR::ttype_t &a, const ASR::ttype_t &b) {
    // TODO: If anyone of the input or argument is derived type then
    // add support for checking member wise types and do not compare
    // directly. From stdlib_string len(pattern) error.
    if (a.type == b.type) {
        // TODO: check dims
        // TODO: check all types
        switch (a.type) {
            case (ASR::ttypeType::Integer) : {
                ASR::Integer_t *a2 = ASR::down_cast<ASR::Integer_t>(&a);
                ASR::Integer_t *b2 = ASR::down_cast<ASR::Integer_t>(&b);
                if (a2->m_kind == b2->m_kind) {
                    return true;
                } else {
                    return false;
                }
                break;
            }
            case (ASR::ttypeType::Real) : {
                ASR::Real_t *a2 = ASR::down_cast<ASR::Real_t>(&a);
                ASR::Real_t *b2 = ASR::down_cast<ASR::Real_t>(&b);
                if (a2->m_kind == b2->m_kind) {
                    return true;
                } else {
                    return false;
                }
                break;
            }
            case (ASR::ttypeType::Complex) : {
                ASR::Complex_t *a2 = ASR::down_cast<ASR::Complex_t>(&a);
                ASR::Complex_t *b2 = ASR::down_cast<ASR::Complex_t>(&b);
                if (a2->m_kind == b2->m_kind) {
                    return true;
                } else {
                    return false;
                }
                break;
            }
            case (ASR::ttypeType::Logical) : {
                ASR::Logical_t *a2 = ASR::down_cast<ASR::Logical_t>(&a);
                ASR::Logical_t *b2 = ASR::down_cast<ASR::Logical_t>(&b);
                if (a2->m_kind == b2->m_kind) {
                    return true;
                } else {
                    return false;
                }
                break;
            }
            case (ASR::ttypeType::Character) : {
                ASR::Character_t *a2 = ASR::down_cast<ASR::Character_t>(&a);
                ASR::Character_t *b2 = ASR::down_cast<ASR::Character_t>(&b);
                if (a2->m_kind == b2->m_kind) {
                    return true;
                } else {
                    return false;
                }
                break;
            }
            case (ASR::ttypeType::List) : {
                ASR::List_t *a2 = ASR::down_cast<ASR::List_t>(&a);
                ASR::List_t *b2 = ASR::down_cast<ASR::List_t>(&b);
                return types_equal(*a2->m_type, *b2->m_type);
            }
            case (ASR::ttypeType::Derived) : {
                ASR::Derived_t *a2 = ASR::down_cast<ASR::Derived_t>(&a);
                ASR::Derived_t *b2 = ASR::down_cast<ASR::Derived_t>(&b);
                ASR::DerivedType_t *a2_type = ASR::down_cast<ASR::DerivedType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    a2->m_derived_type));
                ASR::DerivedType_t *b2_type = ASR::down_cast<ASR::DerivedType_t>(
                                                ASRUtils::symbol_get_past_external(
                                                    b2->m_derived_type));
                return a2_type == b2_type;
            }
            case (ASR::ttypeType::Class) : {
                ASR::Class_t *a2 = ASR::down_cast<ASR::Class_t>(&a);
                ASR::Class_t *b2 = ASR::down_cast<ASR::Class_t>(&b);
                ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
                ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
                if( a2_typesym->type != b2_typesym->type ) {
                    return false;
                }
                if( a2_typesym->type == ASR::symbolType::ClassType ) {
                    ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
                    ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
                    return a2_type == b2_type;
                } else if( a2_typesym->type == ASR::symbolType::DerivedType ) {
                    ASR::DerivedType_t *a2_type = ASR::down_cast<ASR::DerivedType_t>(a2_typesym);
                    ASR::DerivedType_t *b2_type = ASR::down_cast<ASR::DerivedType_t>(b2_typesym);
                    return is_derived_type_similar(a2_type, b2_type);
                }
                return false;
            }
            default : return false;
        }
    } else if( a.type == ASR::ttypeType::Derived &&
               b.type == ASR::ttypeType::Class ) {
        ASR::Derived_t *a2 = ASR::down_cast<ASR::Derived_t>(&a);
        ASR::Class_t *b2 = ASR::down_cast<ASR::Class_t>(&b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_derived_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_class_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::ClassType ) {
            ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
            ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::DerivedType ) {
            ASR::DerivedType_t *a2_type = ASR::down_cast<ASR::DerivedType_t>(a2_typesym);
            ASR::DerivedType_t *b2_type = ASR::down_cast<ASR::DerivedType_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    } else if( a.type == ASR::ttypeType::Class &&
               b.type == ASR::ttypeType::Derived ) {
        ASR::Class_t *a2 = ASR::down_cast<ASR::Class_t>(&a);
        ASR::Derived_t *b2 = ASR::down_cast<ASR::Derived_t>(&b);
        ASR::symbol_t* a2_typesym = ASRUtils::symbol_get_past_external(a2->m_class_type);
        ASR::symbol_t* b2_typesym = ASRUtils::symbol_get_past_external(b2->m_derived_type);
        if( a2_typesym->type != b2_typesym->type ) {
            return false;
        }
        if( a2_typesym->type == ASR::symbolType::ClassType ) {
            ASR::ClassType_t *a2_type = ASR::down_cast<ASR::ClassType_t>(a2_typesym);
            ASR::ClassType_t *b2_type = ASR::down_cast<ASR::ClassType_t>(b2_typesym);
            return a2_type == b2_type;
        } else if( a2_typesym->type == ASR::symbolType::DerivedType ) {
            ASR::DerivedType_t *a2_type = ASR::down_cast<ASR::DerivedType_t>(a2_typesym);
            ASR::DerivedType_t *b2_type = ASR::down_cast<ASR::DerivedType_t>(b2_typesym);
            return is_derived_type_similar(a2_type, b2_type);
        }
    }
    return false;
}

template <typename T>
bool argument_types_match(const Vec<ASR::call_arg_t>& args,
        const T &sub) {
    if (args.size() <= sub.n_args) {
        size_t i;
        for (i = 0; i < args.size(); i++) {
            ASR::Variable_t *v = LFortran::ASRUtils::EXPR2VAR(sub.m_args[i]);
            ASR::ttype_t *arg1 = LFortran::ASRUtils::expr_type(args[i].m_value);
            ASR::ttype_t *arg2 = v->m_type;
            if (!types_equal(*arg1, *arg2)) {
                return false;
            }
        }
        for( ; i < sub.n_args; i++ ) {
            ASR::Variable_t *v = LFortran::ASRUtils::EXPR2VAR(sub.m_args[i]);
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
    LFORTRAN_ASSERT(ASR::is_a<ASR::Function_t>(*final_sym));
    bool is_subroutine = ASR::down_cast<ASR::Function_t>(final_sym)->m_return_var == nullptr;
    ASR::ttype_t *return_type = nullptr;
    if( ASR::is_a<ASR::Function_t>(*final_sym) ) {
        ASR::Function_t* func = ASR::down_cast<ASR::Function_t>(final_sym);
        if (func->m_return_var) {
            if( func->m_elemental && func->n_args == 1 && ASRUtils::is_array(ASRUtils::expr_type(args[0].m_value)) ) {
                return_type = ASRUtils::duplicate_type(al, ASRUtils::expr_type(args[0].m_value));
            } else {
                return_type = LFortran::ASRUtils::EXPR2VAR(func->m_return_var)->m_type;
            }
        }
    }
    // Create ExternalSymbol for the final subroutine:
    // We mangle the new ExternalSymbol's local name as:
    //   generic_procedure_local_name @
    //     specific_procedure_remote_name
    std::string local_sym = std::string(p->m_name) + "@"
        + LFortran::ASRUtils::symbol_name(final_sym);
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
            p->m_module_name, nullptr, 0, LFortran::ASRUtils::symbol_name(final_sym),
            ASR::accessType::Private
            );
        final_sym = ASR::down_cast<ASR::symbol_t>(sub);
        current_scope->add_symbol(local_sym, final_sym);
    } else {
        final_sym = current_scope->get_symbol(local_sym);
    }
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
            // TODO: implement
            // int64_t v = ASR::down_cast<ASR::ConstantInteger_t>(ASRUtils::expr_value(a_arg))->m_n;
            // value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, a_loc, v, a_type));
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

//Initialize pointer to zero so that it can be initialized in first call to get_instance
ASRUtils::LabelGenerator* ASRUtils::LabelGenerator::label_generator = nullptr;

} // namespace ASRUtils


} // namespace LFortran
