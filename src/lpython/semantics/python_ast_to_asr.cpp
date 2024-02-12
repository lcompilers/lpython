#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <cmath>
#include <vector>
#include <bitset>
#include <complex>
#include <sstream>
#include <iterator>

#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <libasr/config.h>
#include <libasr/string_utils.h>
#include <libasr/utils.h>
#include <libasr/pass/instantiate_template.h>
#include <libasr/pass/wrap_global_stmts.h>
#include <libasr/pass/intrinsic_function_registry.h>
#include <libasr/pass/intrinsic_array_function_registry.h>
#include <libasr/modfile.h>
#include <libasr/casting_utils.h>
#include <libasr/codegen/asr_to_fortran.h>
#include <libasr/pickle.h>

#include <lpython/python_ast.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>
#include <lpython/python_serialization.h>
#include <lpython/semantics/python_comptime_eval.h>
#include <lpython/semantics/python_attribute_eval.h>
#include <lpython/semantics/python_intrinsic_eval.h>
#include <lpython/parser/parser.h>
#include <libasr/serialization.h>

namespace LCompilers::LPython {

int save_pyc_files(const ASR::TranslationUnit_t &u,
                       std::string infile) {
    diag::Diagnostics diagnostics;
    LCOMPILERS_ASSERT(asr_verify(u, true, diagnostics));
    std::string modfile_binary = save_pycfile(u);

    while( infile.back() != '.' ) {
        infile.pop_back();
    }
    std::string modfile = infile + "pyc";
    {
        std::ofstream out;
        out.open(modfile, std::ofstream::out | std::ofstream::binary);
        out << modfile_binary;
    }
    return 0;
}

// Does a CPython style lookup for a module:
// * First the current directory (this is incorrect, we need to do it relative to the current file)
// * Then the LPython runtime directory
Result<std::string> get_full_path(const std::string &filename,
        const std::string &runtime_library_dir, std::string& input,
        bool &lpython, bool& enum_py) {
    lpython = false;
    enum_py = false;
    std::string file_path;
    if( *(runtime_library_dir.rbegin()) == '/' ) {
        file_path = runtime_library_dir + filename;
    } else {
        file_path = runtime_library_dir + "/" + filename;
    }
    bool status = read_file(file_path, input);
    if (status) {
        return file_path;
    } else {
        // If this is `lpython`, do a special lookup
        if (filename == "lpython.py") {
            file_path = runtime_library_dir + "/lpython/" + filename;
            status = read_file(file_path, input);
            if (status) {
                lpython = true;
                return file_path;
            } else {
                return Error();
            }
        } else if (startswith(filename, "numpy.py")) {
            file_path = runtime_library_dir + "/lpython_intrinsic_" + filename;
            status = read_file(file_path, input);
            if (status) {
                return file_path;
            } else {
                return Error();
            }
        } else if (startswith(filename, "enum.py")) {
            enum_py = true;
            return Error();
        } else {
            return Error();
        }
    }
}

bool set_module_path(std::string infile0, std::vector<std::string> &rl_path,
                     std::string& infile, std::string& path_used, std::string& input,
                     bool& lpython, bool& enum_py) {
    for (auto path: rl_path) {
        Result<std::string> rinfile = get_full_path(infile0, path, input, lpython, enum_py);
        if (rinfile.ok) {
            infile = rinfile.result;
            path_used = path;
            return true;
        }
    }
    return false;
}

ASR::TranslationUnit_t* compile_module_till_asr(
        Allocator& al, SymbolTable* symtab, std::string module_name,
        std::vector<std::string> &rl_path, std::string infile,
        const Location &loc, diag::Diagnostics &diagnostics, LocationManager &lm,
        const std::function<void (const std::string &, const Location &)> err,
        bool allow_implicit_casting) {
    {
        LocationManager::FileLocations fl;
        fl.in_filename = infile;
        lm.files.push_back(fl);
        std::string input = read_file(infile);
        lm.file_ends.push_back(lm.file_ends.back() + input.size());
        lm.init_simple(input);
    }
    Result<AST::ast_t*> r = parse_python_file(al, rl_path[0], infile,
        diagnostics, lm.file_ends.end()[-2], false);
    if (!r.ok) {
        err("The file '" + infile + "' failed to parse", loc);
    }
    LCompilers::LPython::AST::ast_t* ast = r.result;

    // Convert the module from AST to ASR
    CompilerOptions compiler_options;
    compiler_options.po.disable_main = true;
    compiler_options.symtab_only = false;
    Result<ASR::TranslationUnit_t*> r2 = python_ast_to_asr(al, lm, symtab, *ast,
        diagnostics, compiler_options, false, module_name, infile, allow_implicit_casting);
    // TODO: Uncomment once a check is added for ensuring
    // that module.py file hasn't changed between
    // builds.
    // save_pyc_files(*r2.result, infile + "c");
    if (!r2.ok) {
        LCOMPILERS_ASSERT(diagnostics.has_error())
        return nullptr; // Error
    }

    return r2.result;
}

void fill_module_dependencies(SymbolTable* symtab, SetChar& mod_deps,
    Allocator& al) {
    if( symtab == nullptr ) {
        return ;
    }
    for( auto& itr: symtab->get_scope() ) {
        if( ASR::is_a<ASR::ExternalSymbol_t>(*itr.second) ) {
            ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(itr.second);
            mod_deps.push_back(al, ext_sym->m_module_name);
        } else {
            SymbolTable* sym_symtab = ASRUtils::symbol_symtab(itr.second);
            fill_module_dependencies(sym_symtab, mod_deps, al);
        }
    }
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, diag::Diagnostics &diagnostics,
                            LocationManager &lm, bool intrinsic,
                            std::vector<std::string> &rl_path,
                            bool &lpython, bool& enum_py, bool& copy, bool& sympy,
                            const std::function<void (const std::string &, const Location &)> err,
                            bool allow_implicit_casting) {
    lpython = false;
    enum_py = false;
    copy = false;
    sympy = false;
    if( module_name == "copy" ) {
        copy = true;
        return nullptr;
    }
    if (module_name == "sympy") {
        sympy = true;
        return nullptr;
    }
    LCOMPILERS_ASSERT(symtab);
    if (symtab->get_scope().find(module_name) != symtab->get_scope().end()) {
        ASR::symbol_t *m = symtab->get_symbol(module_name);
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            err("The symbol '" + module_name + "' is not a module", loc);
        }
    }
    LCOMPILERS_ASSERT(symtab->parent == nullptr);

    // Parse the module `module_name`.py to AST
    std::string infile0 = module_name + ".py";
    std::string infile0c = infile0 + "c";
    std::string path_used = "", infile;
    bool compile_module = true;
    ASR::TranslationUnit_t* mod1 = nullptr;
    std::string input;
    std::string mod1_name = "";
    bool found = set_module_path(infile0c, rl_path, infile,
                                 path_used, input, lpython, enum_py);
    if( !found ) {
        input.clear();
        found = set_module_path(infile0, rl_path, infile,
                                path_used, input, lpython, enum_py);
    } else {
        mod1 = load_pycfile(al, input, false);
        fix_external_symbols(*mod1, *ASRUtils::get_tu_symtab(symtab));
        diag::Diagnostics diagnostics;
        LCOMPILERS_ASSERT(asr_verify(*mod1, true, diagnostics));
        compile_module = false;
    }

    if( enum_py ) {
        return nullptr;
    }
    if (!found) {
        err("Could not find the module '" + module_name + "'. If an import path "
            "is available, please use the `-I` option to specify it", loc);
    }
    if (lpython) return nullptr;

    if( compile_module ) {
        diagnostics.add(diag::Diagnostic(
            "The module '" + module_name + "' located in " + infile +" cannot be loaded",
            diag::Level::Warning, diag::Stage::Semantic, {
                diag::Label("imported here", {loc})
            })
        );

        if (module_name == "__init__") {
            std::string module_dir_name = infile.substr(0, infile.find_last_of('/'));
            // assign module directory name
            mod1_name = module_dir_name.substr(module_dir_name.find_last_of('/') + 1);
        } else {
            mod1_name = module_name;
        }
        mod1 = compile_module_till_asr(al, symtab, mod1_name, rl_path, infile, loc, diagnostics,
            lm, err, allow_implicit_casting);
        if (mod1 == nullptr) {
            throw SemanticAbort();
        } else {
            diagnostics.diagnostics.pop_back();
        }
    }

    ASR::symbol_t* mod1_sym = symtab->resolve_symbol(mod1_name);
    if (!mod1_sym) {
        throw SemanticError("Module `" + module_name + "` not found", loc);
    }
    ASR::Module_t* mod1_mod = ASR::down_cast<ASR::Module_t>(mod1_sym);

    if (intrinsic) {
        mod1_mod->m_intrinsic = true;
        // TODO: I think we should just store intrinsic once, in the module
        // itself
        // Mark each function as intrinsic also
        for (auto &item : mod1_mod->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                if (ASRUtils::get_FunctionType(s)->m_abi == ASR::abiType::Source) {
                    ASRUtils::get_FunctionType(s)->m_abi = ASR::abiType::Intrinsic;
                }
                if (s->n_body == 0) {
                    std::string name = s->m_name;
                    if (name == "ubound" || name == "lbound") {
                        ASRUtils::get_FunctionType(s)->m_deftype = ASR::deftypeType::Interface;
                    }
                }
            }
        }
    }

    // and return it
    return mod1_mod;
}

// Here, we call the global_initializer & global_statements to
// initialize and execute the global symbols
void get_calls_to_global_init_and_stmts(Allocator &al, const Location &loc, SymbolTable* scope,
    ASR::Module_t* mod, std::vector<ASR::asr_t *> &tmp_vec) {

    std::string mod_name = mod->m_name;
    std::string g_func_name = mod_name + "global_init";
    ASR::symbol_t *g_func = mod->m_symtab->get_symbol(g_func_name);
    if (g_func && !scope->get_symbol(g_func_name)) {
        ASR::symbol_t *es = ASR::down_cast<ASR::symbol_t>(
            ASR::make_ExternalSymbol_t(al, mod->base.base.loc,
            scope, s2c(al, g_func_name), g_func,
            s2c(al, mod_name), nullptr, 0, s2c(al, g_func_name),
            ASR::accessType::Public));
        scope->add_symbol(g_func_name, es);
        tmp_vec.push_back(ASRUtils::make_SubroutineCall_t_util(al, loc,
            es, g_func, nullptr, 0, nullptr, nullptr, false));
    }

    g_func_name = mod_name + "global_stmts";
    g_func = mod->m_symtab->get_symbol(g_func_name);
    if (g_func && !scope->get_symbol(g_func_name)) {
        ASR::symbol_t *es = ASR::down_cast<ASR::symbol_t>(
            ASR::make_ExternalSymbol_t(al, mod->base.base.loc,
            scope, s2c(al, g_func_name), g_func,
            s2c(al, mod_name), nullptr, 0, s2c(al, g_func_name),
            ASR::accessType::Public));
        scope->add_symbol(g_func_name, es);
        tmp_vec.push_back(ASRUtils::make_SubroutineCall_t_util(al, loc,
            es, g_func, nullptr, 0, nullptr, nullptr, false));
    }
}

template <class Struct>
class CommonVisitor : public AST::BaseVisitor<Struct> {
public:
    diag::Diagnostics &diag;


    ASR::asr_t *tmp;

    /*
    If `tmp` is not null, then `tmp_vec` is ignored and `tmp` is used as the only result (statement or
    expression). If `tmp` is null, then `tmp_vec` is used to return any number of statements:
    0 (no statement returned), 1 (redundant, one should use `tmp` for that), 2, 3, ... etc.
    */
    std::vector<ASR::asr_t *> tmp_vec;

    // Used to store the initializer for the global variables like list, ...
    Vec<ASR::asr_t *> global_init;

    Allocator &al;
    LocationManager &lm;
    SymbolTable *current_scope;
    SetChar current_module_dependencies;
    // True for the main module, false for every other one
    // The main module is stored directly in TranslationUnit, other modules are Modules
    bool main_module;
    // This is the current module name that we are compiling from AST to ASR
    // It is an empty string for main_module
    std::string module_name;
    PythonIntrinsicProcedures intrinsic_procedures;
    ProceduresDatabase procedures_db;
    AttributeHandler attr_handler;
    IntrinsicNodeHandler intrinsic_node_handler;
    std::map<int, ASR::symbol_t*> &ast_overload;
    std::string parent_dir;
    std::vector<std::string> import_paths;
    /*
        current_body exists only for Functions, For, If (& its Else part), While.
        current_body does not exist for Modules, ClassDef/Structs.
    */
    Vec<ASR::stmt_t*> *current_body;
    ASR::expr_t* assign_asr_target;

    std::map<std::string, int> generic_func_nums;
    std::map<std::string, std::map<std::string, ASR::ttype_t*>> generic_func_subs;
    std::vector<ASR::symbol_t*> rt_vec;
    std::map<std::string, std::string> context_map;
    SetChar dependencies;
    bool allow_implicit_casting;
    // Stores the name of imported functions and the modules they are imported from
    std::map<std::string, std::string> imported_functions;
    bool using_args_attr = false;

    std::map<std::string, std::string> numpy2lpythontypes = {
        {"bool", "bool"},
        {"bool_", "bool"},
        {"int8", "i8"},
        {"int16", "i16"},
        {"int32", "i32"},
        {"int64", "i64"},
        {"uint8", "u8"},
        {"uint16", "u16"},
        {"uint32", "u32"},
        {"uint64", "u64"},
        {"float32", "f32"},
        {"float64", "f64"},
        {"float_", "f64"},
        {"complex64", "c32"},
        {"complex128", "c64"},
        {"complex_", "c64"},
        {"object", "T"}
    };

    CommonVisitor(Allocator &al, LocationManager &lm, SymbolTable *symbol_table,
            diag::Diagnostics &diagnostics, bool main_module, std::string module_name,
            std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir,
            std::vector<std::string> import_paths, bool allow_implicit_casting_)
        : diag{diagnostics}, al{al}, lm{lm}, current_scope{symbol_table}, main_module{main_module}, module_name{module_name},
            ast_overload{ast_overload}, parent_dir{parent_dir}, import_paths{import_paths},
            current_body{nullptr}, assign_asr_target{nullptr}, allow_implicit_casting{allow_implicit_casting_} {
        current_module_dependencies.reserve(al, 4);
        global_init.reserve(al, 1);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const std::string &var_name) {
        SymbolTable *scope = current_scope;
        ASR::symbol_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            diag.semantic_error_label("Variable '" + var_name
                + "' is not declared", {loc},
                "'" + var_name + "' is undeclared");
            throw SemanticAbort();
        }
        return ASR::make_Var_t(al, loc, v);
    }

    ASR::symbol_t* resolve_intrinsic_function(const Location &loc, const std::string &remote_sym) {
        LCOMPILERS_ASSERT(intrinsic_procedures.is_intrinsic(remote_sym));
        std::string module_name = intrinsic_procedures.get_module(remote_sym, loc);

        SymbolTable *tu_symtab = ASRUtils::get_tu_symtab(current_scope);
        std::string rl_path = get_runtime_library_dir();
        std::vector<std::string> paths = {rl_path, parent_dir};
        bool lpython, enum_py, copy, sympy;
        ASR::Module_t *m = load_module(al, tu_symtab, module_name,
                loc, diag, lm, true, paths,
                lpython, enum_py, copy, sympy,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); },
                allow_implicit_casting);
        LCOMPILERS_ASSERT(!lpython && !enum_py)

        ASR::symbol_t *t = m->m_symtab->resolve_symbol(remote_sym);
        if (!t) {
            throw SemanticError("The symbol '" + remote_sym
                + "' not found in the module '" + module_name + "'",
                loc);
        } else if (! (ASR::is_a<ASR::GenericProcedure_t>(*t)
                    || ASR::is_a<ASR::Function_t>(*t)
                    )) {
            throw SemanticError("The symbol '" + remote_sym
                + "' found in the module '" + module_name + "', "
                + "but it is not a function, subroutine or a generic procedure.",
                loc);
        }
        char *fn_name = ASRUtils::symbol_name(t);
        std::string sym = fn_name;
        ASR::symbol_t *v = nullptr;
        if( current_scope->get_symbol(sym) == nullptr ) {
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                al, t->base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ fn_name,
                t,
                m->m_name, nullptr, 0, fn_name,
                ASR::accessType::Private
                );
            current_scope->add_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
            v = ASR::down_cast<ASR::symbol_t>(fn);
        } else {
            v = current_scope->get_symbol(sym);
        }

        current_module_dependencies.push_back(al, m->m_name);
        return v;
    }

    void handle_builtin_attribute(ASR::expr_t *s, std::string attr_name,
                const Location &loc, Vec<ASR::expr_t*> &args) {
        tmp = attr_handler.get_attribute(s, attr_name, al, loc, args, diag);
        return;
    }

    void handle_symbolic_attribute(ASR::expr_t *s, std::string attr_name,
                const Location &loc, Vec<ASR::expr_t*> &args) {
        tmp = attr_handler.get_symbolic_attribute(s, attr_name, al, loc, args, diag);
        return;
    }

    void fill_expr_in_ttype_t(std::vector<ASR::expr_t*>& exprs, ASR::dimension_t* dims, size_t n_dims) {
        for( size_t i = 0; i < n_dims; i++ ) {
            exprs.push_back(dims[i].m_start);
            exprs.push_back(dims[i].m_length);
        }
    }

    void fix_exprs_ttype_t(std::vector<ASR::expr_t*>& exprs,
                           Vec<ASR::call_arg_t>& orig_args,
                           ASR::Function_t* orig_func=nullptr) {
        ASRUtils::ExprStmtDuplicator expr_duplicator(al);
        expr_duplicator.allow_procedure_calls = true;
        ASRUtils::ReplaceArgVisitor arg_replacer(al, current_scope, orig_func,
                                                 orig_args, dependencies,
                                                 current_module_dependencies);
        for( size_t i = 0; i < exprs.size(); i++ ) {
            ASR::expr_t* expri = exprs[i];
            if (expri) {
                expr_duplicator.success = true;
                ASR::expr_t* expri_copy = expr_duplicator.duplicate_expr(expri);
                LCOMPILERS_ASSERT(expr_duplicator.success);
                arg_replacer.current_expr = &expri_copy;
                arg_replacer.replace_expr(expri_copy);
                exprs[i] = expri_copy;
            }
        }
    }

    void fill_new_dims(ASR::Array_t* t, const std::vector<ASR::expr_t*>& func_calls,
        Vec<ASR::dimension_t>& new_dims) {
        new_dims.reserve(al, t->n_dims);
        for( size_t i = 0, j = 0; i < func_calls.size(); i += 2, j++ ) {
            ASR::dimension_t new_dim;
            if (func_calls[i] != nullptr) {
                new_dim.loc = func_calls[i]->base.loc;
                new_dim.m_start = func_calls[i];
                new_dim.m_length = func_calls[i + 1];
                new_dims.push_back(al, new_dim);
            } else {
                new_dims.push_back(al, t->m_dims[j]);
            }
        }
    }

    ASR::ttype_t* handle_return_type(ASR::ttype_t *return_type, const Location &loc,
                                     Vec<ASR::call_arg_t>& args,
                                     ASR::Function_t* f=nullptr) {
        // Rebuild the return type if needed and make FunctionCalls use ExternalSymbol
        std::vector<ASR::expr_t*> func_calls;
        switch( return_type->type ) {
            case ASR::ttypeType::Allocatable: {
                ASR::Allocatable_t* allocatable_t = ASR::down_cast<ASR::Allocatable_t>(return_type);
                return ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::type_get_past_pointer(
                            handle_return_type(allocatable_t->m_type, loc, args, f)))));
            }
            case ASR::ttypeType::Pointer: {
                ASR::Pointer_t* pointer_t = ASR::down_cast<ASR::Pointer_t>(return_type);
                return ASRUtils::TYPE(ASR::make_Pointer_t(al, loc,
                    ASRUtils::type_get_past_allocatable(
                        ASRUtils::type_get_past_pointer(
                            handle_return_type(pointer_t->m_type, loc, args, f)))));
            }
            case ASR::ttypeType::Array: {
                ASR::Array_t* t = ASR::down_cast<ASR::Array_t>(return_type);
                ASR::ttype_t* t_m_type = handle_return_type(t->m_type, loc, args, f);
                fill_expr_in_ttype_t(func_calls, t->m_dims, t->n_dims);
                fix_exprs_ttype_t(func_calls, args, f);
                Vec<ASR::dimension_t> new_dims;
                fill_new_dims(t, func_calls, new_dims);
                return ASRUtils::make_Array_t_util(al, loc, t_m_type, new_dims.p, new_dims.size());
            }
            case ASR::ttypeType::Character: {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(return_type);
                func_calls.push_back(t->m_len_expr);
                fix_exprs_ttype_t(func_calls, args, f);
                int64_t a_len = t->m_len;
                if( func_calls[0] ) {
                    a_len = ASRUtils::extract_len<SemanticError>(func_calls[0], loc);
                }
                return ASRUtils::TYPE(ASR::make_Character_t(al, loc, t->m_kind, a_len, func_calls[0]));
            }
            case ASR::ttypeType::Struct: {
                ASR::Struct_t* struct_t_type = ASR::down_cast<ASR::Struct_t>(return_type);
                ASR::symbol_t *sym = struct_t_type->m_derived_type;
                ASR::symbol_t *es_s = current_scope->resolve_symbol(
                    ASRUtils::symbol_name(sym));
                if (es_s == nullptr) {
                    ASR::StructType_t *st = ASR::down_cast<ASR::StructType_t>(sym);
                    ASR::Module_t* sym_module = ASRUtils::get_sym_module(sym);
                    LCOMPILERS_ASSERT(sym_module != nullptr);
                    std::string st_name = "1_" + std::string(st->m_name);
                    if (current_scope->get_symbol(st_name)) {
                        sym = current_scope->get_symbol(st_name);
                    } else {
                        sym = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(
                            al, st->base.base.loc, current_scope, s2c(al, st_name),
                            sym, sym_module->m_name, nullptr, 0, st->m_name,
                            ASR::accessType::Public));
                        current_scope->add_symbol(st_name, sym);
                    }
                } else {
                    sym = es_s;
                }
                return ASRUtils::TYPE(ASR::make_Struct_t(al, loc, sym));
            }
            default: {
                return return_type;
            }
        }
        return nullptr;
    }


    void visit_AsyncFunctionDef(const AST::AsyncFunctionDef_t &x){
        throw SemanticError("The `async` keyword is currently not supported", x.base.base.loc);
    }

    void visit_expr_list(AST::expr_t** exprs, size_t n,
                         Vec<ASR::expr_t*>& exprs_vec) {
        LCOMPILERS_ASSERT(exprs_vec.reserve_called);
        for( size_t i = 0; i < n; i++ ) {
            this->visit_expr(*exprs[i]);
            exprs_vec.push_back(al, ASRUtils::EXPR(tmp));
        }
    }

    void visit_expr_list(AST::expr_t** exprs, size_t n,
                         Vec<ASR::call_arg_t>& call_args_vec) {
        LCOMPILERS_ASSERT(call_args_vec.reserve_called);
        for( size_t i = 0; i < n; i++ ) {
            this->visit_expr(*exprs[i]);
            ASR::expr_t* expr = nullptr;
            ASR::call_arg_t arg;
            arg.loc.first = -1;
            arg.loc.last = -1;
            if (tmp) {
                expr = ASRUtils::EXPR(tmp);
                arg.loc = expr->base.loc;
            }
            arg.m_value = expr;
            call_args_vec.push_back(al, arg);
        }
    }

    int64_t find_argument_position_from_name(ASR::Function_t* orig_func, std::string arg_name,
                                             const Location& call_loc, bool raise_error) {
        int64_t arg_position = -1;
        for( size_t i = 0; i < orig_func->n_args; i++ ) {
            ASR::Var_t* arg_Var = ASR::down_cast<ASR::Var_t>(orig_func->m_args[i]);
            std::string original_arg_name = ASRUtils::symbol_name(arg_Var->m_v);
            if( original_arg_name == arg_name ) {
                return i;
            }
        }
        for ( size_t i = 0; i < ASRUtils::get_FunctionType(orig_func)->n_restrictions; i++ ) {
            ASR::symbol_t* rt = ASRUtils::get_FunctionType(orig_func)->m_restrictions[i];
            std::string rt_name = ASRUtils::symbol_name(rt);
            if ( rt_name == arg_name ) {
                return -2;
            }
        }
        if( raise_error && arg_position == -1 ) {
            throw SemanticError("Function " + std::string(orig_func->m_name) +
                                " doesn't have an argument named '" + arg_name + "'",
                                call_loc);
        }
        return arg_position;
    }

    bool visit_expr_list(AST::expr_t** pos_args, size_t n_pos_args,
                         AST::keyword_t* kwargs, size_t n_kwargs,
                         Vec<ASR::call_arg_t>& call_args_vec,
                         std::map<std::string, ASR::symbol_t*>& rt_subs,
                         ASR::Function_t* orig_func, const Location& call_loc,
                         bool raise_error=true) {
        LCOMPILERS_ASSERT(call_args_vec.reserve_called);

        // Fill the whole call_args_vec with nullptr
        // This is for error handling later on.
        for( size_t i = 0; i < n_pos_args + n_kwargs; i++ ) {
            ASR::call_arg_t call_arg;
            Location loc;
            loc.first = loc.last = 1;
            call_arg.m_value = nullptr;
            call_arg.loc = loc;
            call_args_vec.push_back(al, call_arg);
        }

        // Now handle positional arguments in the following loop
        for( size_t i = 0; i < n_pos_args; i++ ) {
            this->visit_expr(*pos_args[i]);
            ASR::expr_t* expr = ASRUtils::EXPR(tmp);
            call_args_vec.p[i].loc = expr->base.loc;
            call_args_vec.p[i].m_value = expr;
        }

        // Now handle keyword arguments in the following loop
        for( size_t i = 0; i < n_kwargs; i++ ) {
            this->visit_expr(*kwargs[i].m_value);
            ASR::expr_t* expr = ASRUtils::EXPR(tmp);
            std::string arg_name = std::string(kwargs[i].m_arg);
            int64_t arg_pos = find_argument_position_from_name(orig_func, arg_name, call_loc, raise_error);
            if( arg_pos == -1 ) {
                return false;
            }
            // Special treatment for argument to generic function's restriction
            if (arg_pos == -2) {
                if (ASR::is_a<ASR::Var_t>(*expr)) {
                    ASR::Var_t* var = ASR::down_cast<ASR::Var_t>(expr);
                    rt_subs[arg_name] = var->m_v;
                }
                continue;
            }
            if( call_args_vec[arg_pos].m_value != nullptr ) {
                if( !raise_error ) {
                    return false;
                }
                throw SemanticError(std::string(orig_func->m_name) + "() got multiple values for argument '"
                                    + arg_name + "'",
                                    call_loc);
            }
            call_args_vec.p[arg_pos].loc = expr->base.loc;
            call_args_vec.p[arg_pos].m_value = expr;
        }
        return true;
    }

    int64_t find_argument_position_from_name(ASR::StructType_t* orig_struct, std::string arg_name) {
        for( size_t i = 0; i < orig_struct->n_members; i++ ) {
            std::string original_arg_name = std::string(orig_struct->m_members[i]);
            if( original_arg_name == arg_name ) {
                return i;
            }
        }
        return -1;
    }

    void visit_expr_list(AST::expr_t** pos_args, size_t n_pos_args,
                         AST::keyword_t* kwargs, size_t n_kwargs,
                         Vec<ASR::call_arg_t>& call_args_vec,
                         ASR::StructType_t* orig_struct, const Location &loc) {
        LCOMPILERS_ASSERT(call_args_vec.reserve_called);

        // Fill the whole call_args_vec with nullptr
        // This is for error handling later on.
        for( size_t i = 0; i < n_pos_args + n_kwargs; i++ ) {
            ASR::call_arg_t call_arg;
            Location loc;
            loc.first = loc.last = 1;
            call_arg.m_value = nullptr;
            call_arg.loc = loc;
            call_args_vec.push_back(al, call_arg);
        }

        // Now handle positional arguments in the following loop
        for( size_t i = 0; i < n_pos_args; i++ ) {
            this->visit_expr(*pos_args[i]);
            ASR::expr_t* expr = ASRUtils::EXPR(tmp);
            call_args_vec.p[i].loc = expr->base.loc;
            call_args_vec.p[i].m_value = expr;
        }

        // Now handle keyword arguments in the following loop
        for( size_t i = 0; i < n_kwargs; i++ ) {
            this->visit_expr(*kwargs[i].m_value);
            ASR::expr_t* expr = ASRUtils::EXPR(tmp);
            std::string arg_name = std::string(kwargs[i].m_arg);
            int64_t arg_pos = find_argument_position_from_name(orig_struct, arg_name);
            if( arg_pos == -1 ) {
                throw SemanticError("Member '" + arg_name + "' not found in struct", kwargs[i].loc);
            } else if (arg_pos >= (int64_t)call_args_vec.size()) {
                throw SemanticError("Not enough arguments to " + std::string(orig_struct->m_name)
                    + "(), expected " + std::to_string(orig_struct->n_members), loc);
            }
            if( call_args_vec[arg_pos].m_value != nullptr ) {
                throw SemanticError(std::string(orig_struct->m_name) + "() got multiple values for argument '"
                                    + arg_name + "'", kwargs[i].loc);
            }
            call_args_vec.p[arg_pos].loc = expr->base.loc;
            call_args_vec.p[arg_pos].m_value = expr;
        }
    }

    void visit_expr_list_with_cast(ASR::expr_t** m_args, size_t n_args,
                                   Vec<ASR::call_arg_t>& call_args_vec,
                                   Vec<ASR::call_arg_t>& args,
                                   bool check_type_equality=true) {
        LCOMPILERS_ASSERT(call_args_vec.reserve_called);
        for (size_t i = 0; i < n_args; i++) {
            ASR::call_arg_t c_arg;
            c_arg.loc = args[i].loc;
            c_arg.m_value = args[i].m_value;
            cast_helper(m_args[i], c_arg.m_value, true);
            ASR::ttype_t* left_type = ASRUtils::expr_type(m_args[i]);
            ASR::ttype_t* right_type = ASRUtils::expr_type(c_arg.m_value);
            if( check_type_equality && !ASRUtils::check_equal_type(left_type, right_type) ) {
                std::string ltype = ASRUtils::type_to_str_python(left_type);
                std::string rtype = ASRUtils::type_to_str_python(right_type);
                diag.add(diag::Diagnostic(
                    "Type mismatch in procedure call; the types must be compatible",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (passed argument type is " + rtype + " but required type is " + ltype + ")",
                                { c_arg.loc, left_type->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            call_args_vec.push_back(al, c_arg);
        }
    }

    ASR::ttype_t* get_type_from_var_annotation(std::string var_annotation,
        const Location& loc, Vec<ASR::dimension_t>& dims,
        AST::expr_t** m_args=nullptr, [[maybe_unused]] size_t n_args=0,
        bool raise_error=true, ASR::abiType abi=ASR::abiType::Source,
        bool is_argument=false) {
        ASR::ttype_t* type = nullptr;
        ASR::symbol_t *s = current_scope->resolve_symbol(var_annotation);
        if (s) {
            if (ASR::is_a<ASR::Variable_t>(*s)) {
                ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(s);
                if (var_sym->m_type->type == ASR::ttypeType::TypeParameter) {
                    ASR::TypeParameter_t *type_param = ASR::down_cast<ASR::TypeParameter_t>(var_sym->m_type);
                    type = ASRUtils::TYPE(ASR::make_TypeParameter_t(al, loc, type_param->m_param));
                    type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
                }
            } else {
                ASR::symbol_t *der_sym = ASRUtils::symbol_get_past_external(s);
                if( der_sym ) {
                    if ( ASR::is_a<ASR::StructType_t>(*der_sym) ) {
                        type = ASRUtils::TYPE(ASR::make_Struct_t(al, loc, s));
                        type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
                    } else if( ASR::is_a<ASR::EnumType_t>(*der_sym) ) {
                        type = ASRUtils::TYPE(ASR::make_Enum_t(al, loc, s));
                        type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
                    } else if( ASR::is_a<ASR::UnionType_t>(*der_sym) ) {
                        type = ASRUtils::TYPE(ASR::make_Union_t(al, loc, s));
                        type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
                    }
                }
            }
        } else if (var_annotation == "i8") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 1));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "i16") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 2));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "i32") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "i64") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 8));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "u8") {
            type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, loc, 1));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "u16") {
            type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, loc, 2));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "u32") {
            type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, loc, 4));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "u64") {
            type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, loc, 8));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "f32") {
            type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 4));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "f64") {
            type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "c32") {
            type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 4));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "c64") {
            type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc, 8));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "str") {
            type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "bool" || var_annotation == "i1") {
            type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4));
            type = ASRUtils::make_Array_t_util(al, loc, type, dims.p, dims.size(), abi, is_argument);
        } else if (var_annotation == "CPtr") {
            type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        } else if (var_annotation == "pointer") {
            LCOMPILERS_ASSERT(n_args == 1);
            AST::expr_t* underlying_type = m_args[0];
            bool is_allocatable = false;
            type = ast_expr_to_asr_type(underlying_type->base.loc, *underlying_type, is_allocatable);
            type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, type));
        } else if (var_annotation == "S") {
            type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, loc));
        }

        if( !type && raise_error ) {
            if (var_annotation == "int") {
                std::string msg = "Hint: Use i8, i16, i32 or i64 for now. ";
                diag.add(diag::Diagnostic(
                    var_annotation + " type is not supported yet. ",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label(msg, {loc})
                    })
                );
                throw SemanticAbort();
            } else { 
                throw SemanticError("The type '" + var_annotation+"' is undeclared.", loc);
            }
        }

        return type;
    }

    ASR::symbol_t* import_from_module(Allocator &al, ASR::Module_t *m, SymbolTable *current_scope,
                std::string /*mname*/, std::string cur_sym_name, std::string& new_sym_name,
                const Location &loc, bool skip_current_scope_check=false) {
        new_sym_name = ASRUtils::get_mangled_name(m, new_sym_name);
        ASR::symbol_t *t = m->m_symtab->resolve_symbol(cur_sym_name);
        if (!t) {
            throw SemanticError("The symbol '" + cur_sym_name + "' not found in the module '" + std::string(m->m_name) + "'",
                    loc);
        }
        if (!skip_current_scope_check &&
            current_scope->get_scope().find(new_sym_name) != current_scope->get_scope().end()) {
            throw SemanticError(new_sym_name + " already defined", loc);
        }
        if (ASR::is_a<ASR::Function_t>(*t)) {
            ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
            // `mfn` is the Function in a module. Now we construct
            // an ExternalSymbol that points to it.
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)mfn,
                m->m_name, nullptr, 0, mfn->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(fn);
        } else if (ASR::is_a<ASR::StructType_t>(*t)) {
            ASR::StructType_t *st = ASR::down_cast<ASR::StructType_t>(t);
            // `st` is the StructType in a module. Now we construct
            // an ExternalSymbol that points to it.
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *est = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)st,
                m->m_name, nullptr, 0, st->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(est);
        } else if (ASR::is_a<ASR::EnumType_t>(*t)) {
            ASR::EnumType_t *et = ASR::down_cast<ASR::EnumType_t>(t);
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *est = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)et,
                m->m_name, nullptr, 0, et->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(est);
        } else if (ASR::is_a<ASR::UnionType_t>(*t)) {
            ASR::UnionType_t *ut = ASR::down_cast<ASR::UnionType_t>(t);
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *est = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)ut,
                m->m_name, nullptr, 0, ut->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(est);
        } else if (ASR::is_a<ASR::Variable_t>(*t)) {
            ASR::Variable_t *mv = ASR::down_cast<ASR::Variable_t>(t);
            // `mv` is the Variable in a module. Now we construct
            // an ExternalSymbol that points to it.
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)mv,
                m->m_name, nullptr, 0, mv->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(v);
        } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
            ASR::GenericProcedure_t *gt = ASR::down_cast<ASR::GenericProcedure_t>(t);
            Str name;
            name.from_str(al, new_sym_name);
            char *cname = name.c_str(al);
            ASR::asr_t *v = ASR::make_ExternalSymbol_t(
                al, loc,
                /* a_symtab */ current_scope,
                /* a_name */ cname,
                (ASR::symbol_t*)gt,
                m->m_name, nullptr, 0, gt->m_name,
                ASR::accessType::Public
                );
            current_module_dependencies.push_back(al, m->m_name);
            return ASR::down_cast<ASR::symbol_t>(v);
        } else if (ASR::is_a<ASR::ExternalSymbol_t>(*t)) {
            ASR::ExternalSymbol_t *es = ASR::down_cast<ASR::ExternalSymbol_t>(t);
            SymbolTable *symtab = current_scope;
            // while (symtab->parent != nullptr) symtab = symtab->parent;
            ASR::symbol_t *sym = symtab->resolve_symbol(es->m_module_name);
            ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(sym);
            return import_from_module(al, m, symtab, es->m_module_name,
                                cur_sym_name, new_sym_name, loc);
        } else if (ASR::is_a<ASR::Module_t>(*t)) {
            ASR::Module_t* mt = ASR::down_cast<ASR::Module_t>(t);
            std::string mangled_cur_sym_name = ASRUtils::get_mangled_name(mt, new_sym_name);
            if( cur_sym_name == mangled_cur_sym_name ) {
                throw SemanticError("Importing modules isn't supported yet.", loc);
            }
            return import_from_module(al, mt, current_scope, std::string(mt->m_name),
                                    cur_sym_name, new_sym_name, loc);
        } else {
            throw SemanticError("Only Subroutines, Functions, StructType, Variables and "
                "ExternalSymbol are currently supported in 'import'", loc);
        }
        LCOMPILERS_ASSERT(false);
        return nullptr;
    }


    ASR::asr_t* make_dummy_assignment(ASR::expr_t* expr) {
        ASR::ttype_t* type = ASRUtils::expr_type(expr);
        std::string dummy_ret_name = current_scope->get_unique_name("__lcompilers_dummy", false);
        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type);
        ASR::asr_t* variable_asr = ASR::make_Variable_t(al, expr->base.loc, current_scope,
                                        s2c(al, dummy_ret_name), variable_dependencies_vec.p,
                                        variable_dependencies_vec.size(), ASR::intentType::Local,
                                        nullptr, nullptr, ASR::storage_typeType::Default,
                                        type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
                                        ASR::presenceType::Required, false);
        ASR::symbol_t* variable_sym = ASR::down_cast<ASR::symbol_t>(variable_asr);
        current_scope->add_symbol(dummy_ret_name, variable_sym);
        ASR::expr_t* variable_var = ASRUtils::EXPR(ASR::make_Var_t(al, expr->base.loc, variable_sym));
        return ASR::make_Assignment_t(al, expr->base.loc, variable_var, expr, nullptr);
    }

    // Function to create appropriate call based on symbol type. If it is external
    // generic symbol then it changes the name accordingly.
    ASR::asr_t* make_call_helper(Allocator &al, ASR::symbol_t* s, SymbolTable *current_scope,
                    Vec<ASR::call_arg_t> args, std::string call_name, const Location &loc,
                    AST::expr_t** pos_args=nullptr, size_t n_pos_args=0,
                    AST::keyword_t* kwargs=nullptr, size_t n_kwargs=0) {
        if (intrinsic_node_handler.is_present(call_name)) {
            return intrinsic_node_handler.get_intrinsic_node(call_name, al, loc,
                    args);
        }

        if (call_name == "list" && (args.size() == 0 ||  args[0].m_value == nullptr)) {
            if (assign_asr_target) {
                ASR::ttype_t *type = ASRUtils::get_contained_type(
                    ASRUtils::type_get_past_const(ASRUtils::expr_type(assign_asr_target)));
                ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, loc, type));
                Vec<ASR::expr_t*> list;
                list.reserve(al, 1);
                return ASR::make_ListConstant_t(al, loc, list.p,
                    list.size(), list_type);
            }
            return nullptr;
        }

        ASR::symbol_t *s_generic = nullptr, *stemp = s;
        // Type map for generic functions
        std::map<std::string, ASR::ttype_t*> subs;
        std::map<std::string, ASR::symbol_t*> rt_subs;
        // handling ExternalSymbol
        s = ASRUtils::symbol_get_past_external(s);
        bool is_generic_procedure = ASR::is_a<ASR::GenericProcedure_t>(*s);
        if (ASR::is_a<ASR::GenericProcedure_t>(*s)) {
            s_generic = stemp;
            ASR::GenericProcedure_t *p = ASR::down_cast<ASR::GenericProcedure_t>(s);
            int idx = -1;
            if( n_kwargs > 0 ) {
                args.reserve(al, n_pos_args + n_kwargs);
                for( size_t iproc = 0; iproc < p->n_procs; iproc++ ) {
                    args.n = 0;
                    ASR::Function_t* orig_func = ASR::down_cast<ASR::Function_t>(p->m_procs[iproc]);
                    if( !visit_expr_list(pos_args, n_pos_args, kwargs, n_kwargs,
                                         args, rt_subs, orig_func, loc, false) ) {
                        continue;
                    }
                    idx = ASRUtils::select_generic_procedure(args, *p, loc,
                        [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); },
                        false);
                    if( idx == (int) iproc ) {
                        break;
                    }
                }
                if( idx == -1 ) {
                    throw SemanticError("Arguments do not match for any generic procedure, " + std::string(p->m_name), loc);
                }
            } else {
                idx = ASRUtils::select_generic_procedure(args, *p, loc,
                        [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); });
            }
            s = p->m_procs[idx];
            std::string remote_sym = ASRUtils::symbol_name(s);
            std::string local_sym = ASRUtils::symbol_name(s);
            if (ASR::is_a<ASR::ExternalSymbol_t>(*stemp)) {
                local_sym = std::string(p->m_name) + "@" + local_sym;
            }

            SymbolTable *symtab = current_scope;
            if (symtab->resolve_symbol(local_sym) == nullptr) {
                LCOMPILERS_ASSERT(ASR::is_a<ASR::ExternalSymbol_t>(*stemp));
                std::string mod_name = ASR::down_cast<ASR::ExternalSymbol_t>(stemp)->m_module_name;
                ASR::symbol_t *mt = symtab->resolve_symbol(mod_name);
                ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(mt);
                local_sym = ASRUtils::get_mangled_name(m, local_sym);
                stemp = import_from_module(al, m, symtab, mod_name,
                                    remote_sym, local_sym, loc);
                LCOMPILERS_ASSERT(ASR::is_a<ASR::ExternalSymbol_t>(*stemp));
                symtab->add_symbol(local_sym, stemp);
                s = ASRUtils::symbol_get_past_external(stemp);
            } else {
                stemp = symtab->resolve_symbol(local_sym);
            }
        }
        if (ASR::is_a<ASR::Function_t>(*s)) {
            ASR::Function_t *func = ASR::down_cast<ASR::Function_t>(s);
            if( n_kwargs > 0 && !is_generic_procedure ) {
                args.reserve(al, n_pos_args + n_kwargs);
                visit_expr_list(pos_args, n_pos_args, kwargs, n_kwargs,
                                args, rt_subs, func, loc);
            }
            if (ASRUtils::get_FunctionType(func)->m_is_restriction) {
                rt_vec.push_back(s);
            } else if (ASRUtils::is_generic_function(s)) {
                if (n_pos_args != func->n_args) {
                    std::string fnd = std::to_string(n_pos_args);
                    std::string org = std::to_string(func->n_args);
                    diag.add(diag::Diagnostic(
                        "Number of arguments does not match in the function call",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("(found: '" + fnd + "', expected: '" + org + "')",
                                    {loc})
                        })
                    );
                    throw SemanticAbort();
                }
                for (size_t i=0; i<n_pos_args; i++) {
                    ASR::ttype_t *param_type = ASRUtils::expr_type(func->m_args[i]);
                    ASR::ttype_t *arg_type = ASRUtils::expr_type(args[i].m_value);
                    check_type_substitution(subs, param_type, arg_type, loc);
                }
                for (size_t i=0; i<ASRUtils::get_FunctionType(func)->n_restrictions; i++) {
                    ASR::Function_t* rt = ASR::down_cast<ASR::Function_t>(
                        ASRUtils::get_FunctionType(func)->m_restrictions[i]);
                    check_type_restriction(subs, rt_subs, rt, loc);
                }


                //ASR::symbol_t *t = get_generic_function(subs, rt_subs, func);
                ASR::symbol_t *t = get_generic_function(subs, rt_subs, s);
                std::string new_call_name = (ASR::down_cast<ASR::Function_t>(t))->m_name;


                // Currently ignoring keyword arguments for generic function calls
                Vec<ASR::call_arg_t> new_args;
                new_args.reserve(al, n_pos_args);
                for (size_t i = 0; i<n_pos_args; i++) {
                    ASR::call_arg_t new_call_arg;
                    new_call_arg.m_value = args.p[i].m_value;
                    new_call_arg.loc = args.p[i].loc;
                    new_args.push_back(al, new_call_arg);
                }
                return make_call_helper(al, t, current_scope, new_args, new_call_name, loc);
            }
            if (args.size() != func->n_args) {
                std::string fnd = std::to_string(args.size());
                std::string org = std::to_string(func->n_args);
                diag.add(diag::Diagnostic(
                    "Number of arguments does not match in the function call",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("(found: '" + fnd + "', expected: '" + org + "')",
                                {loc})
                    })
                );
                throw SemanticAbort();
            }
            if (ASR::down_cast<ASR::Function_t>(s)->m_return_var != nullptr) {
                ASR::ttype_t *a_type = nullptr;
                if( ASRUtils::get_FunctionType(func)->m_elemental && args.size() == 1 &&
                    ASRUtils::is_array(ASRUtils::expr_type(args[0].m_value)) ) {
                    a_type = ASRUtils::expr_type(args[0].m_value);
                } else {
                    a_type = ASRUtils::expr_type(func->m_return_var);
                    a_type = handle_return_type(a_type, loc, args, func);
                }
                ASR::expr_t *value = nullptr;
                if (ASRUtils::is_intrinsic_function2(func)) {
                    value = intrinsic_procedures.comptime_eval(call_name, al, loc, args);
                }

                Vec<ASR::call_arg_t> args_new;
                args_new.reserve(al, func->n_args);
                visit_expr_list_with_cast(func->m_args, func->n_args, args_new, args,
                    !ASRUtils::is_intrinsic_function2(func));

                if (ASRUtils::symbol_parent_symtab(stemp)->get_counter() != current_scope->get_counter()) {
                    ADD_ASR_DEPENDENCIES(current_scope, stemp, dependencies);
                }

                return ASRUtils::make_FunctionCall_t_util(al, loc, stemp,
                                                s_generic, args_new.p, args_new.size(),
                                                a_type, value, nullptr);
            } else {
                Vec<ASR::call_arg_t> args_new;
                args_new.reserve(al, func->n_args);
                visit_expr_list_with_cast(func->m_args, func->n_args, args_new, args);

                if (ASRUtils::symbol_parent_symtab(stemp)->get_counter() != current_scope->get_counter()) {
                    ADD_ASR_DEPENDENCIES(current_scope, stemp, dependencies);
                }

                return ASRUtils::make_SubroutineCall_t_util(al, loc, stemp,
                    s_generic, args_new.p, args_new.size(), nullptr, nullptr, false);
            }
        } else if(ASR::is_a<ASR::StructType_t>(*s)) {
            ASR::StructType_t* StructType = ASR::down_cast<ASR::StructType_t>(s);
            if (n_kwargs > 0) {
                args.reserve(al, n_pos_args + n_kwargs);
                visit_expr_list(pos_args, n_pos_args, kwargs, n_kwargs,
                                         args, StructType, loc);
            }

            if (args.size() > 0 && args.size() > StructType->n_members) {
                throw SemanticError("Struct constructor has more arguments than the number of struct members",
                                    loc);
            }

            for( size_t i = 0; i < args.size(); i++ ) {
                std::string member_name = StructType->m_members[i];
                ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(
                                                StructType->m_symtab->resolve_symbol(member_name));
                ASR::expr_t* arg_new_i = args[i].m_value;
                cast_helper(member_var->m_type, arg_new_i, arg_new_i->base.loc);
                ASR::ttype_t* left_type = member_var->m_type;
                ASR::ttype_t* right_type = ASRUtils::expr_type(arg_new_i);
                if( !ASRUtils::check_equal_type(left_type, right_type) ) {
                    std::string ltype = ASRUtils::type_to_str_python(left_type);
                    std::string rtype = ASRUtils::type_to_str_python(right_type);
                    diag.add(diag::Diagnostic(
                        "Type mismatch in procedure call; the types must be compatible",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("type mismatch (passed argument type is " + rtype + " but required type is " + ltype + ")",
                                    { arg_new_i->base.loc, left_type->base.loc})
                        })
                    );
                    throw SemanticAbort();
                }
                args.p[i].m_value = arg_new_i;
            }
            for (size_t i = args.size(); i < StructType->n_members; i++) {
                args.push_back(al, StructType->m_initializers[i]);
            }
            ASR::ttype_t* der_type = ASRUtils::TYPE(ASR::make_Struct_t(al, loc, stemp));
            return ASR::make_StructTypeConstructor_t(al, loc, stemp, args.p, args.size(), der_type, nullptr);
        } else if( ASR::is_a<ASR::EnumType_t>(*s) ) {
            Vec<ASR::expr_t*> args_new;
            args_new.reserve(al, args.size());
            ASRUtils::visit_expr_list(al, args, args_new);
            ASR::EnumType_t* enumtype = ASR::down_cast<ASR::EnumType_t>(s);
            for( size_t i = 0; i < std::min(args.size(), enumtype->n_members); i++ ) {
                std::string member_name = enumtype->m_members[i];
                ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(
                                                enumtype->m_symtab->resolve_symbol(member_name));
                ASR::expr_t* arg_new_i = args_new[i];
                cast_helper(member_var->m_type, arg_new_i, arg_new_i->base.loc);
                args_new.p[i] = arg_new_i;
            }
            ASR::ttype_t* der_type = ASRUtils::TYPE(ASR::make_Enum_t(al, loc, s));
            return ASR::make_EnumTypeConstructor_t(al, loc, stemp, args_new.p, args_new.size(), der_type, nullptr);
        } else if( ASR::is_a<ASR::UnionType_t>(*s) ) {
            if( args.size() != 0 ) {
                throw SemanticError("Union constructors do not accept any argument as of now.",
                    loc);
            }
            ASR::ttype_t* union_ = ASRUtils::TYPE(ASR::make_Union_t(al, loc, stemp));
            return ASR::make_UnionTypeConstructor_t(al, loc, stemp, nullptr, 0, union_, nullptr);
        } else {
            throw SemanticError("Unsupported call type for " + call_name, loc);
        }
    }

    /**
     * @brief Check if the type of the argument given does not contradict
     *        with previously checked type substitution.
     */
    void check_type_substitution(std::map<std::string, ASR::ttype_t*>& subs,
            ASR::ttype_t *param_type, ASR::ttype_t *arg_type, const Location &loc) {
        if (ASR::is_a<ASR::List_t>(*param_type)) {
            if (ASR::is_a<ASR::List_t>(*arg_type)) {
                ASR::ttype_t *param_elem = ASR::down_cast<ASR::List_t>(param_type)->m_type;
                ASR::ttype_t *arg_elem = ASR::down_cast<ASR::List_t>(arg_type)->m_type;
                return check_type_substitution(subs, param_elem, arg_elem, loc);
            } else {
                throw SemanticError("The parameter is a list while the argument is not a list", loc);
            }
        }

        ASR::ttype_t* param_type_ = ASRUtils::type_get_past_array(param_type);
        if (ASR::is_a<ASR::TypeParameter_t>(*param_type_)) {
            ASR::TypeParameter_t *tp = ASR::down_cast<ASR::TypeParameter_t>(param_type_);
            std::string param_name = tp->m_param;
            if (subs.find(param_name) != subs.end()) {
                if (!ASRUtils::check_equal_type(subs[param_name], arg_type)) {
                    throw SemanticError("Inconsistent type variable for the function call", loc);
                }
            } else {
                if (ASRUtils::is_array(param_type) && ASRUtils::is_array(arg_type)) {
                    ASR::dimension_t* dims = nullptr;
                    int param_dims = ASRUtils::extract_dimensions_from_ttype(param_type, dims);
                    int arg_dims = ASRUtils::extract_dimensions_from_ttype(arg_type, dims);
                    if (param_dims == arg_dims) {
                        subs[param_name] = ASRUtils::duplicate_type_without_dims(al, arg_type, arg_type->base.loc);
                    } else {
                        throw SemanticError("Inconsistent type subsititution for array type", loc);
                    }
                } else {
                    subs[param_name] = ASRUtils::duplicate_type(al, arg_type);
                }
            }
        }
    }

    /**
     * @brief Check if the given function and the type substitution satisfies
     *        the restriction
     */
    void check_type_restriction(std::map<std::string, ASR::ttype_t*> subs,
            std::map<std::string, ASR::symbol_t*> rt_subs,
            ASR::Function_t* rt, const Location& loc) {
        std::string rt_name = rt->m_name;
        if (rt_subs.find(rt_name) != rt_subs.end()) {
            ASR::symbol_t* rt_arg = rt_subs[rt_name];
            if (!ASR::is_a<ASR::Function_t>(*rt_arg)) {
                std::string msg = "The restriction " + rt_name + " requires a "
                    + "function as an argument";
                throw SemanticError(msg, loc);
            }
            ASR::Function_t* rt_arg_func = ASR::down_cast<ASR::Function_t>(rt_arg);
            /** @brief different argument number between the function given and the
             *         restriction result in error **/
            if (rt->n_args != rt_arg_func->n_args) {
                std::string msg = "The function " + std::string(rt_arg_func->m_name)
                    + " provided for the restriction "
                    + std::string(rt->m_name) + " have different number of arguments";
                throw SemanticError(msg, rt_arg->base.loc);
            }
            for (size_t j=0; j<rt->n_args; j++) {
                ASR::ttype_t* rt_type = ASRUtils::expr_type(rt->m_args[j]);
                ASR::ttype_t* rt_arg_type = ASRUtils::expr_type(rt_arg_func->m_args[j]);
                if (ASRUtils::is_type_parameter(*rt_type)) {
                    std::string rt_type_param = ASR::down_cast<ASR::TypeParameter_t>(
                        ASRUtils::get_type_parameter(rt_type))->m_param;
                    /**
                     * @brief if the type of the function given for the restriction does not
                     *        satisfy the type substitution from the function argument, it
                     *        results in error **/
                    if (!ASRUtils::check_equal_type(subs[rt_type_param], rt_arg_type)) {
                        throw SemanticError("Restriction mismatch with provided arguments",
                            rt_arg->base.loc);
                    }
                }
            }
            if (rt->m_return_var) {
                if (!rt_arg_func->m_return_var) {
                    throw SemanticError("The function provided to the restriction should "
                        "have a return value", rt_arg->base.loc);
                }
                ASR::ttype_t* rt_return = ASRUtils::expr_type(rt->m_return_var);
                ASR::ttype_t* rt_arg_return = ASRUtils::expr_type(rt_arg_func->m_return_var);
                if (ASRUtils::is_type_parameter(*rt_return)) {
                    std::string rt_return_param = ASR::down_cast<ASR::TypeParameter_t>(
                        ASRUtils::get_type_parameter(rt_return))->m_param;
                    if (!ASRUtils::check_equal_type(subs[rt_return_param], rt_arg_return)) {
                        throw SemanticError("Restriction mismatch with provided arguments",
                            rt_arg->base.loc);
                    }
                }
            } else {
                if (rt_arg_func->m_return_var) {
                    throw SemanticError("The function provided to the restriction should "
                        "not have a return value", rt_arg->base.loc);
                }
            }
            return;
        }
        /**
         * @brief Check if there is not argument given to the restriction
         *  plus : integer x integer -> integer, real x real -> real
         *  zero : integer -> integer, real -> real
         *  div  : integer x i32 -> f64, real x i32 -> f64
         */
        if (rt_name == "add" && rt->n_args == 2) {
            ASR::ttype_t* left_type = ASRUtils::expr_type(rt->m_args[0]);
            ASR::ttype_t* right_type = ASRUtils::expr_type(rt->m_args[1]);
            left_type = ASR::is_a<ASR::TypeParameter_t>(*left_type)
                ? subs[ASR::down_cast<ASR::TypeParameter_t>(left_type)->m_param] : left_type;
            right_type = ASR::is_a<ASR::TypeParameter_t>(*right_type)
                ? subs[ASR::down_cast<ASR::TypeParameter_t>(right_type)->m_param] : right_type;
            if ((ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) ||
                    (ASRUtils::is_real(*left_type) && ASRUtils::is_real(*right_type))) {
                return;
            }
        } else if (rt_name == "zero" && rt->n_args == 1) {
            ASR::ttype_t* type = ASRUtils::expr_type(rt->m_args[0]);
            type = ASR::is_a<ASR::TypeParameter_t>(*type)
                ? subs[ASR::down_cast<ASR::TypeParameter_t>(type)->m_param] : type;
            if (ASRUtils::is_integer(*type) || ASRUtils::is_real(*type)) {
                return;
            }
        } else if (rt_name == "div" && rt->n_args == 2) {
            ASR::ttype_t* left_type = ASRUtils::expr_type(rt->m_args[0]);
            ASR::ttype_t* right_type = ASRUtils::expr_type(rt->m_args[1]);
            left_type = ASR::is_a<ASR::TypeParameter_t>(*left_type)
                ? subs[ASR::down_cast<ASR::TypeParameter_t>(left_type)->m_param] : left_type;
            if ((ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) ||
                    (ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type))) {
                return;
            }
        }
        throw SemanticError("No applicable argument to the restriction " + rt_name , loc);
    }

    /**
     * @brief Check if the generic function has been instantiated with similar
     *        arguments. If not, then instantiate a new function.
     */
    ASR::symbol_t* get_generic_function(std::map<std::string, ASR::ttype_t*> subs,
            std::map<std::string, ASR::symbol_t*>& rt_subs, ASR::symbol_t *sym) {
        int new_function_num;
        ASR::symbol_t *t;
        std::string func_name = ASRUtils::symbol_name(sym);
        if (generic_func_nums.find(func_name) != generic_func_nums.end()) {
            new_function_num = generic_func_nums[func_name];
            for (int i=0; i<generic_func_nums[func_name]; i++) {
                std::string generic_func_name = "__asr_generic_" + func_name + "_" + std::to_string(i);
                if (generic_func_subs.find(generic_func_name) != generic_func_subs.end()) {
                    std::map<std::string, ASR::ttype_t*> subs_check = generic_func_subs[generic_func_name];
                    if (subs_check.size() != subs.size()) { continue; }
                    bool defined = true;
                    for (auto const &subs_check_pair: subs_check) {
                        if (subs.find(subs_check_pair.first) == subs.end()) {
                            defined = false; break;
                        }
                        ASR::ttype_t* subs_type = subs[subs_check_pair.first];
                        ASR::ttype_t* subs_check_type = subs_check_pair.second;
                        if (!ASRUtils::check_equal_type(subs_type, subs_check_type)) {
                            defined = false; break;
                        }
                    }
                    if (defined) {
                        t = current_scope->resolve_symbol(generic_func_name);
                        return t;
                    }
                }
            }
        } else {
            new_function_num = 0;
        }
        generic_func_nums[func_name] = new_function_num + 1;
        std::string new_func_name = "__asr_generic_" + func_name + "_"
            + std::to_string(new_function_num);
        generic_func_subs[new_func_name] = subs;
        SymbolTable *target_scope = ASRUtils::symbol_parent_symtab(sym);
        t = instantiate_symbol(al, context_map, subs, rt_subs,
                target_scope, target_scope, new_func_name, sym);
        if (ASR::is_a<ASR::Function_t>(*sym)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(sym);
            ASR::Function_t *new_f = ASR::down_cast<ASR::Function_t>(t);
            t = instantiate_function_body(al, context_map, subs, rt_subs,
                target_scope, target_scope, new_f, f);
        }
        dependencies.erase(s2c(al, func_name));

        if (ASRUtils::symbol_parent_symtab(sym)->get_counter() != current_scope->get_counter()) {
            ADD_ASR_DEPENDENCIES_WITH_NAME(current_scope, sym, dependencies, s2c(al, new_func_name));
        }
        return t;
    }

    bool contains_local_variable(ASR::expr_t* value) {
        if( ASR::is_a<ASR::Var_t>(*value) ) {
            ASR::Var_t* var_value = ASR::down_cast<ASR::Var_t>(value);
            ASR::symbol_t* var_value_sym = var_value->m_v;
            ASR::Variable_t* var_value_variable = ASR::down_cast<ASR::Variable_t>(
                ASRUtils::symbol_get_past_external(var_value_sym));
            return var_value_variable->m_intent == ASR::intentType::Local;
        }

        // TODO: Let any other expression pass through
        // Will be handled later
        return false;
    }

    void fill_dims_for_asr_type(Vec<ASR::dimension_t>& dims,
                                ASR::expr_t* value, const Location& loc,
                                bool is_allocatable=false) {
        ASR::dimension_t dim;
        dim.loc = loc;
        if (ASR::is_a<ASR::IntegerConstant_t>(*value) ||
            ASR::is_a<ASR::Var_t>(*value) ||
            ASR::is_a<ASR::IntegerBinOp_t>(*value)) {
            ASR::ttype_t *itype = ASRUtils::expr_type(value);
            ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, itype));
            ASR::expr_t* zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 0, itype));
            ASR::expr_t* comptime_val = nullptr;
            int64_t value_int = -1;
            if( !ASRUtils::extract_value(ASRUtils::expr_value(value), value_int) &&
                contains_local_variable(value) && !is_allocatable) {
                throw SemanticError("Only those local variables that can be reduced to compile-time constants should be used in dimensions of an array.",
                                    value->base.loc);
            }
            if (value_int != -1) {
                comptime_val = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, value_int - 1, itype));
            }
            dim.m_start = zero;
            dim.m_length = ASRUtils::compute_length_from_start_end(al, dim.m_start,
                            ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, value->base.loc,
                                value, ASR::binopType::Sub, one, itype, comptime_val)));
            dims.push_back(al, dim);
        } else if(ASR::is_a<ASR::TupleConstant_t>(*value)) {
            ASR::TupleConstant_t* tuple_constant = ASR::down_cast<ASR::TupleConstant_t>(value);
            for( size_t i = 0; i < tuple_constant->n_elements; i++ ) {
                ASR::expr_t *value = tuple_constant->m_elements[i];
                fill_dims_for_asr_type(dims, value, loc, is_allocatable);
            }
        } else if(ASR::is_a<ASR::ListConstant_t>(*value)) {
            ASR::ListConstant_t* list_constant = ASR::down_cast<ASR::ListConstant_t>(value);
            for( size_t i = 0; i < list_constant->n_args; i++ ) {
                ASR::expr_t *value = list_constant->m_args[i];
                fill_dims_for_asr_type(dims, value, loc, is_allocatable);
            }
        } else if(ASR::is_a<ASR::EnumValue_t>(*value)) {
            ASR::expr_t* enum_value = ASRUtils::expr_value(
                 ASR::down_cast<ASR::EnumValue_t>(value)->m_value);
            if (!enum_value) {
                throw SemanticError("Only constant enumeration values are "
                                    "supported as array dimensions.", loc);
            }
            fill_dims_for_asr_type(dims, enum_value, loc, is_allocatable);
        } else {
            throw SemanticError("Only Integer, `:` or identifier in [] in "
                                "Subscript supported for now in annotation "
                                "found, " + std::to_string(value->type),
                loc);
        }
    }

    bool is_runtime_array(AST::expr_t* m_slice) {
        if( AST::is_a<AST::Tuple_t>(*m_slice) ) {
            AST::Tuple_t* multidim = AST::down_cast<AST::Tuple_t>(m_slice);
            for( size_t i = 0; i < multidim->n_elts; i++ ) {
                if( AST::is_a<AST::Slice_t>(*multidim->m_elts[i]) ) {
                    return true;
                }
            }
        }
        return false;
    }

    void raise_error_when_dict_key_is_float_or_complex(ASR::ttype_t* key_type, const Location& loc) {
        if(ASR::is_a<ASR::Real_t>(*key_type) || ASR::is_a<ASR::Complex_t>(*key_type)) {
            throw SemanticError("'dict' key type cannot be float/complex because resolving collisions "
                                "by exact comparison of float/complex values will result in unexpected "
                                "behaviours. In addition fuzzy equality checks with a certain tolerance "
                                "does not follow transitivity with float/complex values.", loc);
        }
    }

    AST::expr_t* get_var_intent_and_annotation(AST::expr_t *annotation, ASR::intentType &intent) {
        if (AST::is_a<AST::Subscript_t>(*annotation)) {
            AST::Subscript_t *s = AST::down_cast<AST::Subscript_t>(annotation);
            if (AST::is_a<AST::Name_t>(*s->m_value)) {
                std::string ann_name = AST::down_cast<AST::Name_t>(s->m_value)->m_id;
                if (ann_name == "In") {
                    intent = ASRUtils::intent_in;
                    return s->m_slice;
                } else if (ann_name == "InOut") {
                    intent = ASRUtils::intent_inout;
                    return s->m_slice;
                } else if (ann_name == "Out") {
                    intent = ASRUtils::intent_out;
                    return s->m_slice;
                }
                return annotation;
            } else {
                throw SemanticError("Only Name in Subscript supported for now in annotation", annotation->base.loc);
            }
        }
        return annotation;
    }

    // Convert Python AST type annotation to an ASR type
    // Examples:
    // i32, i64, f32, f64
    // f64[256], i32[:]
    ASR::ttype_t * ast_expr_to_asr_type(const Location &loc, const AST::expr_t &annotation,
        bool &is_allocatable, bool raise_error=true, ASR::abiType abi=ASR::abiType::Source,
        bool is_argument=false) {
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 4);
        AST::expr_t** m_args = nullptr; size_t n_args = 0;

        std::string var_annotation;
        if (AST::is_a<AST::Name_t>(annotation)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(&annotation);
            var_annotation = n->m_id;
            return get_type_from_var_annotation(var_annotation,
                annotation.base.loc, dims, m_args, n_args, raise_error,
                abi, is_argument);
        }

        if (AST::is_a<AST::ConstantNone_t>(annotation)) {
            return nullptr;
        }

        if (AST::is_a<AST::Subscript_t>(annotation)) {
            AST::Subscript_t *s = AST::down_cast<AST::Subscript_t>(&annotation);
            if (AST::is_a<AST::Name_t>(*s->m_value)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(s->m_value);
                var_annotation = n->m_id;
            } else {
                throw SemanticError("Only Name in Subscript supported for now in annotation",
                    loc);
            }

            if (var_annotation == "In" || var_annotation == "InOut" || var_annotation == "Out") {
                throw SemanticError("Intent annotation '" + var_annotation + "' cannot be used here", s->base.base.loc);
            }

            if (var_annotation == "tuple") {
                Vec<ASR::ttype_t*> types;
                types.reserve(al, 4);
                if (AST::is_a<AST::Name_t>(*s->m_slice)) {
                    types.push_back(al, ast_expr_to_asr_type(loc, *s->m_slice,
                        is_allocatable, raise_error, abi, is_argument));
                } else if (AST::is_a<AST::Tuple_t>(*s->m_slice)) {
                    AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                    for (size_t i=0; i<t->n_elts; i++) {
                        types.push_back(al, ast_expr_to_asr_type(loc, *t->m_elts[i],
                            is_allocatable, raise_error, abi, is_argument));
                    }
                } else {
                    throw SemanticError("Only Name or Tuple in Subscript supported for now in `tuple` annotation",
                        loc);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
                    types.p, types.size()));
                return type;
            } else if (var_annotation == "Callable") {
                LCOMPILERS_ASSERT(AST::is_a<AST::Tuple_t>(*s->m_slice));
                AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                LCOMPILERS_ASSERT(t->n_elts <= 2 && t->n_elts >= 1);
                Vec<ASR::ttype_t*> arg_types;
                LCOMPILERS_ASSERT(AST::is_a<AST::List_t>(*t->m_elts[0]));

                AST::List_t *arg_list = AST::down_cast<AST::List_t>(t->m_elts[0]);
                if (arg_list->n_elts > 0) {
                    arg_types.reserve(al, arg_list->n_elts);
                    for (size_t i=0; i<arg_list->n_elts; i++) {
                        arg_types.push_back(al, ast_expr_to_asr_type(loc, *arg_list->m_elts[i],
                                                is_allocatable, raise_error, abi, is_argument));
                    }
                } else {
                    arg_types.reserve(al, 1);
                }
                ASR::ttype_t* ret_type = nullptr;
                if (t->n_elts == 2) {
                    ret_type = ast_expr_to_asr_type(loc, *t->m_elts[1],
                        is_allocatable, raise_error, abi, is_argument);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_FunctionType_t(al, loc, arg_types.p,
                        arg_types.size(), ret_type, ASR::abiType::Source,
                        ASR::deftypeType::Interface, nullptr, false, false,
                        false, false, false, nullptr, 0, false));
                return type;
            } else if (var_annotation == "set") {
                if (AST::is_a<AST::Name_t>(*s->m_slice) || AST::is_a<AST::Subscript_t>(*s->m_slice)) {
                    ASR::ttype_t *type = ast_expr_to_asr_type(loc, *s->m_slice,
                        is_allocatable, raise_error, abi, is_argument);
                    return ASRUtils::TYPE(ASR::make_Set_t(al, loc, type));
                } else {
                    throw SemanticError("Only Name in Subscript supported for now in `set`"
                        " annotation", loc);
                }
            } else if (var_annotation == "list") {
                ASR::ttype_t *type = nullptr;
                if (AST::is_a<AST::Name_t>(*s->m_slice) || AST::is_a<AST::Subscript_t>(*s->m_slice)) {
                    type = ast_expr_to_asr_type(loc, *s->m_slice,
                        is_allocatable, raise_error, abi, is_argument);
                    return ASRUtils::TYPE(ASR::make_List_t(al, loc, type));
                } else {
                    throw SemanticError("Only Name or Subscript inside Subscript supported for now in `list`"
                        " annotation", loc);
                }
            } else if (var_annotation == "Allocatable") {
                ASR::ttype_t *type = nullptr;
                if (AST::is_a<AST::Name_t>(*s->m_slice) || AST::is_a<AST::Subscript_t>(*s->m_slice)) {
                    type = ast_expr_to_asr_type(loc, *s->m_slice,
                        is_allocatable, raise_error, abi, is_argument);
                    is_allocatable = true;
                    return type;
                } else {
                    throw SemanticError("Only Name or Subscript inside Subscript supported for now in `list`"
                        " annotation", loc);
                }
            } else if (var_annotation == "dict") {
                if (AST::is_a<AST::Tuple_t>(*s->m_slice)) {
                    AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                    if (t->n_elts != 2) {
                        throw SemanticError("`dict` annotation must have 2 elements: types"
                            " of both keys and values", loc);
                    }
                    ASR::ttype_t *key_type = ast_expr_to_asr_type(loc, *t->m_elts[0],
                        is_allocatable, raise_error, abi, is_argument);
                    ASR::ttype_t *value_type = ast_expr_to_asr_type(loc, *t->m_elts[1],
                        is_allocatable, raise_error, abi, is_argument);
                    raise_error_when_dict_key_is_float_or_complex(key_type, loc);
                    return ASRUtils::TYPE(ASR::make_Dict_t(al, loc, key_type, value_type));
                } else {
                    throw SemanticError("`dict` annotation must have 2 elements: types of"
                        " both keys and values", loc);
                }
            } else if (var_annotation == "Pointer") {
                ASR::ttype_t *type = ast_expr_to_asr_type(loc, *s->m_slice,
                    is_allocatable, raise_error, abi, is_argument);
                return ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, type));
            } else if (var_annotation == "Const") {
                ASR::ttype_t *type = ast_expr_to_asr_type(loc, *s->m_slice,
                    is_allocatable, raise_error, abi, is_argument);
                return ASRUtils::TYPE(ASR::make_Const_t(al, loc, type));
            } else {
                AST::expr_t* dim_info = s->m_slice;

                if (var_annotation == "Array") {
                    LCOMPILERS_ASSERT(AST::is_a<AST::Tuple_t>(*s->m_slice));
                    AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                    LCOMPILERS_ASSERT(t->n_elts >= 2);
                    LCOMPILERS_ASSERT(AST::is_a<AST::Name_t>(*t->m_elts[0]));
                    var_annotation = AST::down_cast<AST::Name_t>(t->m_elts[0])->m_id;
                    Vec<AST::expr_t*> dims;
                    dims.reserve(al, 0);
                    for (size_t i = 1; i < t->n_elts; i++) {
                        dims.push_back(al, t->m_elts[i]);
                    }
                    AST::ast_t* dim_tuple = AST::make_Tuple_t(al, t->base.base.loc, dims.p, dims.size(),
                        AST::expr_contextType::Load);
                    dim_info = AST::down_cast<AST::expr_t>(dim_tuple);
                }

                ASR::ttype_t* type = get_type_from_var_annotation(var_annotation,
                    annotation.base.loc, dims, m_args, n_args, raise_error, abi, is_argument);

                if (AST::is_a<AST::Slice_t>(*dim_info)) {
                    ASR::dimension_t dim;
                    dim.loc = loc;
                    dim.m_start = nullptr;
                    dim.m_length = nullptr;
                    dims.push_back(al, dim);
                } else if( is_runtime_array(dim_info) ) {
                    AST::Tuple_t* tuple_multidim = AST::down_cast<AST::Tuple_t>(dim_info);
                    for( size_t i = 0; i < tuple_multidim->n_elts; i++ ) {
                        if( AST::is_a<AST::Slice_t>(*tuple_multidim->m_elts[i]) ) {
                            ASR::dimension_t dim;
                            dim.loc = loc;
                            dim.m_start = nullptr;
                            dim.m_length = nullptr;
                            dims.push_back(al, dim);
                        }
                    }
                } else {
                    this->visit_expr(*dim_info);
                    ASR::expr_t *value = ASRUtils::EXPR(tmp);
                    fill_dims_for_asr_type(dims, value, loc);
                }

                if (ASRUtils::ttype_set_dimensions(&type, dims.p, dims.size(), al, abi, is_argument)) {
                    return type;
                }

                throw SemanticError("ICE: Unable to set dimensions for: " + var_annotation, loc);
            }
        } else if (AST::is_a<AST::Attribute_t>(annotation)) {
            AST::Attribute_t* attr_annotation = AST::down_cast<AST::Attribute_t>(&annotation);
            LCOMPILERS_ASSERT(AST::is_a<AST::Name_t>(*attr_annotation->m_value));
            std::string value = AST::down_cast<AST::Name_t>(attr_annotation->m_value)->m_id;
            ASR::symbol_t *t = current_scope->resolve_symbol(value);

            if (!t) {
                throw SemanticError("'" + value + "' is not defined in the scope",
                    attr_annotation->base.base.loc);
            }
            LCOMPILERS_ASSERT(ASR::is_a<ASR::StructType_t>(*t));
            ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(t);
            std::string struct_var_name = struct_type->m_name;
            std::string struct_member_name = attr_annotation->m_attr;
            ASR::symbol_t* struct_member = struct_type->m_symtab->resolve_symbol(struct_member_name);
            if( !struct_member ) {
                throw SemanticError(struct_member_name + " not present in " +
                                    struct_var_name + " dataclass.",
                                    attr_annotation->base.base.loc);
            }
            std::string import_name = struct_var_name + "_" + struct_member_name;
            ASR::symbol_t* import_struct_member = current_scope->resolve_symbol(import_name);
            bool import_from_struct = true;
            if( import_struct_member ) {
                if( ASR::is_a<ASR::ExternalSymbol_t>(*import_struct_member) ) {
                    ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(import_struct_member);
                    if( ext_sym->m_external == struct_member &&
                        std::string(ext_sym->m_module_name) == struct_var_name ) {
                        import_from_struct = false;
                    }
                }
            }
            if( import_from_struct ) {
                import_name = current_scope->get_unique_name(import_name, false);
                import_struct_member = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                                        attr_annotation->base.base.loc, current_scope, s2c(al, import_name),
                                                        struct_member,s2c(al, struct_var_name), nullptr, 0,
                                                        s2c(al, struct_member_name), ASR::accessType::Public));
                current_scope->add_symbol(import_name, import_struct_member);
            }
            return ASRUtils::TYPE(ASR::make_Union_t(al, attr_annotation->base.base.loc, import_struct_member));
        }

        throw SemanticError("Only Name, Subscript, and Call supported for now in annotation of annotated assignment.", loc);
    }

    ASR::expr_t *index_add_one(const Location &loc, ASR::expr_t *idx) {
        // Add 1 to the index `idx`, assumes `idx` is of type Integer 4
        ASR::expr_t *comptime_value = nullptr;
        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, loc, 1, a_type));
        return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, idx,
            ASR::binopType::Add, constant_one, a_type, comptime_value));
    }

    void cast_helper(ASR::expr_t*& left, ASR::expr_t*& right, bool is_assign,
        bool is_explicit_casting=false) {
        if( !allow_implicit_casting && !is_explicit_casting ) {
            if( is_assign ) {
                ASR::ttype_t* left_type = ASRUtils::expr_type(left);
                ASR::ttype_t* right_type = ASRUtils::expr_type(right);
                if( ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
                    throw SemanticError("Assigning integer to float is not supported",
                                        right->base.loc);
                }
                if ( ASRUtils::is_complex(*left_type) && !ASRUtils::is_complex(*right_type)) {
                    throw SemanticError("Assigning non-complex to complex is not supported",
                            right->base.loc);
                }
            }
            return ;
        }
        bool no_cast = ((ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(left)) &&
                        ASR::is_a<ASR::Var_t>(*left)) ||
                        (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(right)) &&
                        ASR::is_a<ASR::Var_t>(*right)));
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        if( ASR::is_a<ASR::Const_t>(*left_type) ) {
            left_type = ASRUtils::get_contained_type(left_type);
        }
        if( ASR::is_a<ASR::Const_t>(*right_type) ) {
            right_type = ASRUtils::get_contained_type(right_type);
        }
        left_type = ASRUtils::type_get_past_pointer(left_type);
        right_type = ASRUtils::type_get_past_pointer(right_type);
        if( no_cast ) {
            int lkind = ASRUtils::extract_kind_from_ttype_t(left_type);
            int rkind = ASRUtils::extract_kind_from_ttype_t(right_type);
            if( left_type->type != right_type->type || lkind != rkind ) {
                throw SemanticError("Casting for mismatching pointer types not supported yet.",
                                    right_type->base.loc);
            }
        }

        // Handle presence of logical types in binary operations
        // by converting them into 32-bit integers.
        // See integration_tests/test_bool_binop.py for its significance.
        if(!is_assign && ASRUtils::is_logical(*left_type) && ASRUtils::is_logical(*right_type) ) {
            ASR::ttype_t* dest_type = ASRUtils::TYPE(ASR::make_Integer_t(al, left_type->base.loc, 4));
            left = CastingUtil::perform_casting(left, left_type, dest_type, al, left->base.loc);
            right = CastingUtil::perform_casting(right, right_type, dest_type, al, right->base.loc);
            return ;
        }

        ASR::ttype_t *src_type = nullptr, *dest_type = nullptr;
        ASR::expr_t *src_expr = nullptr, *dest_expr = nullptr;
        int casted_expression_signal = CastingUtil::get_src_dest(left, right, src_expr, dest_expr,
                                                                 src_type, dest_type, is_assign);
        if( casted_expression_signal == 2 ) {
            return ;
        }
        src_expr = CastingUtil::perform_casting(src_expr, src_type,
                                                dest_type, al, src_expr->base.loc);
        if( casted_expression_signal == 0 ) {
            left = src_expr;
            right = dest_expr;
        } else if( casted_expression_signal == 1 ) {
            left = dest_expr;
            right = src_expr;
        }
    }

    void cast_helper(ASR::ttype_t* dest_type, ASR::expr_t*& src_expr,
        const Location& loc, bool is_explicit_cast=false) {
        if( !allow_implicit_casting && !is_explicit_cast ) {
            return ;
        }
        ASR::ttype_t* src_type = ASRUtils::expr_type(src_expr);
        if( ASR::is_a<ASR::Const_t>(*src_type) ) {
            src_type = ASRUtils::get_contained_type(src_type);
        }
        if( ASRUtils::check_equal_type(src_type, dest_type) ) {
            return ;
        }
        src_expr = CastingUtil::perform_casting(src_expr, src_type,
                                                dest_type, al, loc);
    }

    void make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
        ASR::binopType op, const Location &loc) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        if( ASR::is_a<ASR::Const_t>(*left_type) ) {
            left_type = ASRUtils::get_contained_type(left_type);
        }
        if( ASR::is_a<ASR::Const_t>(*right_type) ) {
            right_type = ASRUtils::get_contained_type(right_type);
        }
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;
        ASR::expr_t *overloaded = nullptr;

        bool right_is_int = ASRUtils::is_character(*left_type) && ASRUtils::is_integer(*right_type);
        bool left_is_int = ASRUtils::is_integer(*left_type) && ASRUtils::is_character(*right_type);

        // Handle normal division in python with reals
        if (op == ASR::binopType::Div && ((!ASR::is_a<ASR::SymbolicExpression_t>(*left_type)) ||
            (!ASR::is_a<ASR::SymbolicExpression_t>(*right_type)))) {
            if (ASRUtils::is_character(*left_type) || ASRUtils::is_character(*right_type)) {
                diag.add(diag::Diagnostic(
                    "Division is not supported for string type",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("string not supported in division" ,
                                {left->base.loc, right->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            if(!ASRUtils::check_equal_type(left_type, right_type)){
                std::string ltype = ASRUtils::type_to_str_python(ASRUtils::expr_type(left));
                std::string rtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(right));
                diag.add(diag::Diagnostic(
                    "Type mismatch in binary operator; the types must be compatible",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                                {left->base.loc, right->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            ASR::ttype_t* left_type = ASRUtils::expr_type(left);
            ASR::ttype_t* right_type = ASRUtils::expr_type(right);
            ASR::dimension_t *m_dims_left = nullptr, *m_dims_right = nullptr;
            int n_dims_left = ASRUtils::extract_dimensions_from_ttype(left_type, m_dims_left);
            int n_dims_right = ASRUtils::extract_dimensions_from_ttype(right_type, m_dims_right);
            if( n_dims_left == 0 && n_dims_right == 0 ) {
                int left_type_priority = CastingUtil::get_type_priority(left_type->type);
                int right_type_priority = CastingUtil::get_type_priority(right_type->type);
                int left_kind = ASRUtils::extract_kind_from_ttype_t(left_type);
                int right_kind = ASRUtils::extract_kind_from_ttype_t(right_type);
                bool is_left_f32 = ASR::is_a<ASR::Real_t>(*left_type) && left_kind == 4;
                bool is_right_f32 = ASR::is_a<ASR::Real_t>(*right_type) && right_kind == 4;
                if( (left_type_priority >= right_type_priority && is_left_f32) ||
                    (right_type_priority >= left_type_priority && is_right_f32) ) {
                    dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 4));
                } else if( left_type_priority <= CastingUtil::get_type_priority(ASR::ttypeType::Real) &&
                            right_type_priority <= CastingUtil::get_type_priority(ASR::ttypeType::Real)) {
                    dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8));
                } else {
                    if( left_type_priority > right_type_priority ) {
                        dest_type = ASRUtils::duplicate_type_without_dims(al, left_type, loc);
                    } else if( left_type_priority < right_type_priority ) {
                        dest_type = ASRUtils::duplicate_type_without_dims(al, right_type, loc);
                    } else {
                        if( left_kind >= right_kind ) {
                            dest_type = ASRUtils::duplicate_type_without_dims(al, left_type, loc);
                        } else {
                            dest_type = ASRUtils::duplicate_type_without_dims(al, right_type, loc);
                        }
                    }
                }
                cast_helper(dest_type, left, left->base.loc, true);
                double val = -1.0;
                if (ASRUtils::extract_value(ASRUtils::expr_value(right), val) &&
                    val == 0.0) {
                    diag.add(diag::Diagnostic(
                        "division by zero is not allowed",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("division by zero",
                                    {right->base.loc})
                        })
                    );
                    throw SemanticAbort();
                }
                cast_helper(dest_type, right, right->base.loc, true);
            } else {
                if( n_dims_left != 0 && n_dims_right != 0 ) {
                    LCOMPILERS_ASSERT(n_dims_left == n_dims_right);
                    dest_type = left_type;
                } else {
                    if( n_dims_left > 0 ) {
                        dest_type = left_type;
                    } else {
                        dest_type = right_type;
                    }
                }
            }
        } else if((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type) ||
                    ASRUtils::is_complex(*left_type) || ASRUtils::is_logical(*left_type)) &&
                (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type) ||
                    ASRUtils::is_complex(*right_type) || ASRUtils::is_logical(*right_type))) {
            cast_helper(left, right, false,
                ASRUtils::is_logical(*left_type) && ASRUtils::is_logical(*right_type));
            dest_type = ASRUtils::expr_type(left);
            if( ASR::is_a<ASR::Const_t>(*dest_type) ) {
                dest_type = ASRUtils::get_contained_type(dest_type);
            }
        } else if(ASRUtils::is_unsigned_integer(*left_type)
                 && ASRUtils::is_unsigned_integer(*right_type)) {
            dest_type = ASRUtils::expr_type(left);
            if( ASR::is_a<ASR::Const_t>(*dest_type) ) {
                dest_type = ASRUtils::get_contained_type(dest_type);
            }
        } else if (ASR::is_a<ASR::List_t>(*left_type) && ASRUtils::is_integer(*right_type)
                   && op == ASR::binopType::Mul) {
            dest_type = left_type;
            tmp = ASR::make_ListRepeat_t(al, loc, left, right, dest_type, value);
            return;
        } else if (ASRUtils::is_integer(*left_type) && ASR::is_a<ASR::List_t>(*right_type)
                   && op == ASR::binopType::Mul) {
            dest_type = right_type;
            tmp = ASR::make_ListRepeat_t(al, loc, right, left, dest_type, value);
            return;
        } else if ((right_is_int || left_is_int) && op == ASR::binopType::Mul) {
            // string repeat
            int64_t left_int = 0, right_int = 0, dest_len = 0;
            if (right_is_int) {
                ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(
                    ASRUtils::type_get_past_array(left_type));
                LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(left_type) == 0);
                right_int = ASR::down_cast<ASR::IntegerConstant_t>(
                                                   ASRUtils::expr_value(right))->m_n;
                dest_len = left_type2->m_len * right_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, left_type2->m_kind,
                        dest_len, nullptr));
            } else if (left_is_int) {
                ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(
                    ASRUtils::type_get_past_array(right_type));
                LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(right_type) == 0);
                left_int = ASR::down_cast<ASR::IntegerConstant_t>(
                                                   ASRUtils::expr_value(left))->m_n;
                dest_len = right_type2->m_len * left_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, right_type2->m_kind,
                        dest_len, nullptr));
            }

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* str = right_is_int ? ASR::down_cast<ASR::StringConstant_t>(
                                                ASRUtils::expr_value(left))->m_s :
                                                ASR::down_cast<ASR::StringConstant_t>(
                                                ASRUtils::expr_value(right))->m_s;
                int64_t repeat = right_is_int ? right_int : left_int;
                char* result;
                std::ostringstream os;
                std::fill_n(std::ostream_iterator<std::string>(os), repeat, std::string(str));
                result = s2c(al, os.str());
                LCOMPILERS_ASSERT((int64_t)strlen(result) == dest_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(
                    al, loc, result, dest_type));
            }
            if (right_is_int) {
                tmp = ASR::make_StringRepeat_t(al, loc, left, right, dest_type, value);
            }
            else if (left_is_int){
                tmp = ASR::make_StringRepeat_t(al, loc, right, left, dest_type, value);
            }
            return;

        } else if (ASRUtils::is_character(*left_type) && ASRUtils::is_character(*right_type)
                            && op == ASR::binopType::Add) {
            // string concat
            ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(
                   ASRUtils::type_get_past_array(left_type));
            ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(
                   ASRUtils::type_get_past_array(right_type));
            LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(left_type) == 0);
            LCOMPILERS_ASSERT(ASRUtils::extract_n_dims_from_ttype(right_type) == 0);
            dest_type = ASR::down_cast<ASR::ttype_t>(
                    ASR::make_Character_t(al, loc, left_type2->m_kind,
                    left_type2->m_len + right_type2->m_len, nullptr));
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* left_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(right))->m_s;
                char* result;
                std::string result_s = std::string(left_value) + std::string(right_value);
                result = s2c(al, result_s);
                LCOMPILERS_ASSERT((int64_t)strlen(result) == ASR::down_cast<ASR::Character_t>(dest_type)->m_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(
                    al, loc, result, dest_type));
            }
            tmp = ASR::make_StringConcat_t(al, loc, left, right, dest_type, value);
            return;
        } else if (ASR::is_a<ASR::List_t>(*left_type) && ASR::is_a<ASR::List_t>(*right_type)
                   && op == ASR::binopType::Add) {
            dest_type = left_type;
            std::string ltype = ASRUtils::type_to_str_python(left_type);
            std::string rtype = ASRUtils::type_to_str_python(right_type);
            ASR::ttype_t *left_type2 = ASR::down_cast<ASR::List_t>(left_type)->m_type;
            ASR::ttype_t *right_type2 = ASR::down_cast<ASR::List_t>(right_type)->m_type;
            if (!ASRUtils::check_equal_type(left_type2, right_type2)) {
                diag.add(diag::Diagnostic(
                    "Both the lists should be of the same type for concatenation.",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                                {left->base.loc, right->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            tmp = ASR::make_ListConcat_t(al, loc, left, right, dest_type, value);
            return;
        } else if (ASR::is_a<ASR::Tuple_t>(*left_type) && ASR::is_a<ASR::Tuple_t>(*right_type)
                   && op == ASR::binopType::Add) {
            Vec<ASR::ttype_t*> tuple_type_vec;
            ASR::Tuple_t* tuple_type_left = ASR::down_cast<ASR::Tuple_t>(left_type);
            ASR::Tuple_t* tuple_type_right = ASR::down_cast<ASR::Tuple_t>(right_type);
            tuple_type_vec.reserve(al, tuple_type_left->n_type + tuple_type_right->n_type);
            for (size_t i=0; i<tuple_type_left->n_type; i++) {
                tuple_type_vec.push_back(al, tuple_type_left->m_type[i]);
            }
            for (size_t i=0; i<tuple_type_right->n_type; i++) {
                tuple_type_vec.push_back(al, tuple_type_right->m_type[i]);
            }
            ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
                                        tuple_type_vec.p, tuple_type_vec.n));
            tmp = ASR::make_TupleConcat_t(al, loc, left, right, tuple_type, value);
            return;
        } else if (ASR::is_a<ASR::SymbolicExpression_t>(*left_type)
                   && ASR::is_a<ASR::SymbolicExpression_t>(*right_type)) {
            Vec<ASR::expr_t*> args_with_symbolic;
            args_with_symbolic.reserve(al, 2);
            args_with_symbolic.push_back(al, left);
            args_with_symbolic.push_back(al, right);
            ASRUtils::create_intrinsic_function create_function;
            switch (op) {
                case (ASR::binopType::Add): {
                    create_function = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("SymbolicAdd");
                    break;
                }
                case (ASR::binopType::Sub): {
                    create_function = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("SymbolicSub");
                    break;
                }
                case (ASR::binopType::Mul): {
                    create_function = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("SymbolicMul");
                    break;
                }
                case (ASR::binopType::Div): {
                    create_function = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("SymbolicDiv");
                    break;
                }
                case (ASR::binopType::Pow): {
                    create_function = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("SymbolicPow");
                    break;
                }
                default: {
                    throw SemanticError("Not implemented: The following symbolic binary operator has not been implemented", loc);
                    break;
                }
            }
            tmp = create_function(al, loc, args_with_symbolic, [&](const std::string& msg, const Location& loc) {
                throw SemanticError(msg, loc);
                });
            return;
        } else {
            std::string ltype = ASRUtils::type_to_str_python(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Type mismatch in binary operator; the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }

        left_type = ASRUtils::expr_type(left);
        right_type = ASRUtils::expr_type(right);
        if( !ASRUtils::check_equal_type(left_type, right_type) ) {
            std::string ltype = ASRUtils::type_to_str_python(left_type);
            std::string rtype = ASRUtils::type_to_str_python(right_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in binary operator; the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }

        if (ASRUtils::is_integer(*dest_type)) {
            ASR::dimension_t *m_dims_left = nullptr, *m_dims_right = nullptr;
            int n_dims_left = ASRUtils::extract_dimensions_from_ttype(left_type, m_dims_left);
            int n_dims_right = ASRUtils::extract_dimensions_from_ttype(right_type, m_dims_right);
            if( !(n_dims_left == 0 && n_dims_right == 0) ) {
                if( n_dims_left != 0 && n_dims_right != 0 ) {
                    LCOMPILERS_ASSERT(n_dims_left == n_dims_right);
                    dest_type = left_type;
                } else {
                    if( n_dims_left > 0 ) {
                        dest_type = left_type;
                    } else {
                        dest_type = right_type;
                    }
                }
            }

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                                    ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                                    ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    case (ASR::binopType::BitAnd): { result = left_value & right_value; break; }
                    case (ASR::binopType::BitOr): { result = left_value | right_value; break; }
                    case (ASR::binopType::BitXor): { result = left_value ^ right_value; break; }
                    case (ASR::binopType::BitLShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value << right_value;
                        break;
                    }
                    case (ASR::binopType::BitRShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value >> right_value;
                        break;
                    }
                    default: { throw SemanticError("ICE: Unknown binary operator", loc); } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                    al, loc, result, dest_type));
            }

            tmp = ASR::make_IntegerBinOp_t(al, loc, left, op, right, dest_type, value);

        } else if (ASRUtils::is_unsigned_integer(*dest_type)) {
            ASR::dimension_t *m_dims_left = nullptr, *m_dims_right = nullptr;
            int n_dims_left = ASRUtils::extract_dimensions_from_ttype(left_type, m_dims_left);
            int n_dims_right = ASRUtils::extract_dimensions_from_ttype(right_type, m_dims_right);
            if( !(n_dims_left == 0 && n_dims_right == 0) ) {
                if( n_dims_left != 0 && n_dims_right != 0 ) {
                    LCOMPILERS_ASSERT(n_dims_left == n_dims_right);
                    dest_type = left_type;
                } else {
                    if( n_dims_left > 0 ) {
                        dest_type = left_type;
                    } else {
                        dest_type = right_type;
                    }
                }
            }

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                                    ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                                    ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    case (ASR::binopType::BitAnd): { result = left_value & right_value; break; }
                    case (ASR::binopType::BitOr): { result = left_value | right_value; break; }
                    case (ASR::binopType::BitXor): { result = left_value ^ right_value; break; }
                    case (ASR::binopType::BitLShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value << right_value;
                        break;
                    }
                    case (ASR::binopType::BitRShift): {
                        if (right_value < 0) {
                            throw SemanticError("Negative shift count not allowed.", loc);
                        }
                        result = left_value >> right_value;
                        break;
                    }
                    default: { throw SemanticError("ICE: Unknown binary operator", loc); } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_UnsignedIntegerConstant_t(
                    al, loc, result, dest_type));
            }

            tmp = ASR::make_UnsignedIntegerBinOp_t(al, loc, left, op, right, dest_type, value);

        } else if (ASRUtils::is_real(*dest_type)) {

            if (op == ASR::binopType::BitAnd || op == ASR::binopType::BitOr || op == ASR::binopType::BitXor ||
                op == ASR::binopType::BitLShift || op == ASR::binopType::BitRShift) {
                throw SemanticError("Unsupported binary operation on floats: '" + ASRUtils::binop_to_str_python(op) + "'", loc);
            }

            ASR::dimension_t *m_dims_left = nullptr, *m_dims_right = nullptr;
            int n_dims_left = ASRUtils::extract_dimensions_from_ttype(left_type, m_dims_left);
            int n_dims_right = ASRUtils::extract_dimensions_from_ttype(right_type, m_dims_right);
            if( !(n_dims_left == 0 && n_dims_right == 0) ) {
                if( n_dims_left != 0 && n_dims_right != 0 ) {
                    LCOMPILERS_ASSERT(n_dims_left == n_dims_right);
                    dest_type = left_type;
                } else {
                    if( n_dims_left > 0 ) {
                        dest_type = left_type;
                    } else {
                        dest_type = right_type;
                    }
                }
            }
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                double left_value = ASR::down_cast<ASR::RealConstant_t>(
                                                    ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::RealConstant_t>(
                                                    ASRUtils::expr_value(right))->m_r;
                double result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    default: { throw SemanticError("ICE: Unknown binary operator", loc); }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(
                    al, loc, result, dest_type));
            }

            tmp = ASR::make_RealBinOp_t(al, loc, left, op, right, dest_type, value);

        } else if (ASRUtils::is_complex(*dest_type)) {

            if (op == ASR::binopType::BitAnd || op == ASR::binopType::BitOr || op == ASR::binopType::BitXor ||
                op == ASR::binopType::BitLShift || op == ASR::binopType::BitRShift) {
                throw SemanticError("Unsupported binary operation on complex: '" + ASRUtils::binop_to_str_python(op) + "'", loc);
            }

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                ASR::ComplexConstant_t *left0 = ASR::down_cast<ASR::ComplexConstant_t>(
                                                                ASRUtils::expr_value(left));
                ASR::ComplexConstant_t *right0 = ASR::down_cast<ASR::ComplexConstant_t>(
                                                                ASRUtils::expr_value(right));
                std::complex<double> left_value(left0->m_re, left0->m_im);
                std::complex<double> right_value(right0->m_re, right0->m_im);
                std::complex<double> result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    default: { LCOMPILERS_ASSERT(false); }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ComplexConstant_t(al, loc,
                        std::real(result), std::imag(result), dest_type));
            }

            tmp = ASR::make_ComplexBinOp_t(al, loc, left, op, right, dest_type, value);

        }


        if (overloaded != nullptr) {
            tmp = ASR::make_OverloadedBinOp_t(al, loc, left, op, right, dest_type, value, overloaded);
        }
    }

    bool is_bindc_class(AST::expr_t** decorators, size_t n) {
        for( size_t i = 0; i < n; i++ ) {
            AST::expr_t* decorator = decorators[0];
            if( !AST::is_a<AST::Name_t>(*decorator) ) {
                return false;
            }

            AST::Name_t* dec_name = AST::down_cast<AST::Name_t>(decorator);
            if (std::string(dec_name->m_id) == "ccallable" ||
                std::string(dec_name->m_id) == "ccall") {
                return true;
            }
        }
        return false;
    }

    bool is_dataclass(AST::expr_t** decorators, size_t n,
        ASR::expr_t*& aligned_expr, bool& is_packed) {
        bool is_dataclass_ = false;
        for( size_t i = 0; i < n; i++ ) {
            if( AST::is_a<AST::Name_t>(*decorators[i]) ) {
                AST::Name_t* dc_name = AST::down_cast<AST::Name_t>(decorators[i]);
                is_dataclass_ = std::string(dc_name->m_id) == "dataclass";
                is_packed = is_packed || std::string(dc_name->m_id) == "packed";
            } else if( AST::is_a<AST::Call_t>(*decorators[i]) ) {
                AST::Call_t* dc_call = AST::down_cast<AST::Call_t>(decorators[i]);
                AST::Name_t* dc_name = AST::down_cast<AST::Name_t>(dc_call->m_func);
                if( std::string(dc_name->m_id) == "packed" ) {
                    is_packed = true;
                    if( dc_call->n_keywords == 1 && dc_call->n_args == 0 ) {
                        AST::keyword_t kwarg = dc_call->m_keywords[0];
                        std::string kwarg_name = kwarg.m_arg;
                        if( kwarg_name == "aligned" ) {
                            this->visit_expr(*kwarg.m_value);
                            aligned_expr = ASRUtils::EXPR(tmp);
                            ASR::expr_t* aligned_expr_value = ASRUtils::expr_value(aligned_expr);
                            std::string msg = "Alignment should always evaluate to a constant expressions.";
                            if( !aligned_expr_value ) {
                                throw SemanticError(msg, aligned_expr->base.loc);
                            }
                            int64_t alignment_int;
                            if( !ASRUtils::extract_value(aligned_expr_value, alignment_int) ) {
                                throw SemanticError(msg, aligned_expr->base.loc);
                            }
                            if( alignment_int == 0 || ((alignment_int & (alignment_int - 1)) != 0) ) {
                                throw SemanticError("Alignment " + std::to_string(alignment_int) +
                                                    " is not a positive power of 2.",
                                                    aligned_expr->base.loc);
                            }
                        }
                    }
                }
            }
        }

        return is_dataclass_;
    }

    bool is_enum(AST::expr_t** bases, size_t n) {
        if( n != 1 ) {
            return false;
        }

        AST::expr_t* base = bases[0];
        if( !AST::is_a<AST::Name_t>(*base) ) {
            return false;
        }

        AST::Name_t* base_name = AST::down_cast<AST::Name_t>(base);
        return std::string(base_name->m_id) == "Enum";
    }

    bool is_union(AST::expr_t** bases, size_t n) {
        if( n != 1 ) {
            return false;
        }

        AST::expr_t* base = bases[0];
        if( !AST::is_a<AST::Name_t>(*base) ) {
            return false;
        }

        AST::Name_t* base_name = AST::down_cast<AST::Name_t>(base);
        return std::string(base_name->m_id) == "Union";
    }

    void process_variable_init_val(ASR::symbol_t* v_sym, const Location& loc, ASR::expr_t* init_expr=nullptr) {
        ASR::expr_t* value = nullptr;
        ASR::Variable_t* v_variable = ASR::down_cast<ASR::Variable_t>(v_sym);
        std::string var_name = v_variable->m_name;
        ASR::ttype_t* type = v_variable->m_type;
        if( init_expr ) {
            value = ASRUtils::expr_value(init_expr);
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type, init_expr, value);
            v_variable->m_dependencies = variable_dependencies_vec.p;
            v_variable->n_dependencies = variable_dependencies_vec.size();
            v_variable->m_symbolic_value = init_expr;
            v_variable->m_value = value;
        }

        bool is_runtime_expression = !ASRUtils::is_value_constant(value);
        bool is_variable_const = ASR::is_a<ASR::Const_t>(*type);

        if( is_variable_const && !init_expr ) {
            throw SemanticError("Constant variable " + var_name +
                " is not initialised at declaration.", loc);
        }

        if( init_expr && (current_body || ASR::is_a<ASR::List_t>(*type) ||
                is_runtime_expression) && !is_variable_const) {
            ASR::expr_t* v_expr = ASRUtils::EXPR(ASR::make_Var_t(al, loc, v_sym));
            cast_helper(v_expr, init_expr, true);
            ASR::asr_t* assign = ASR::make_Assignment_t(al, loc, v_expr,
                                                        init_expr, nullptr);
            if (current_body) {
                current_body->push_back(al, ASRUtils::STMT(assign));
            } else if (ASR::is_a<ASR::List_t>(*type) || is_runtime_expression) {
                global_init.push_back(al, assign);
            }

            v_variable->m_symbolic_value = nullptr;
            v_variable->m_value = nullptr;
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type);
            v_variable->m_dependencies = variable_dependencies_vec.p;
            v_variable->n_dependencies = variable_dependencies_vec.size();
        }

        // Restrict this check only to symbols which have a body.
        if( !((v_variable->m_symbolic_value == nullptr && v_variable->m_value == nullptr) ||
              (v_variable->m_symbolic_value != nullptr && v_variable->m_value != nullptr)) &&
              current_body ) {
            throw SemanticError("Initialisation of " + var_name + " must reduce to a compile time constant.", loc);
        }
    }

    void create_add_variable_to_scope(std::string& var_name,
        ASR::ttype_t* type, const Location& loc, ASR::abiType abi,
        ASR::storage_typeType storage_type=ASR::storage_typeType::Default) {

        ASR::intentType s_intent = ASRUtils::intent_local;
        if( ASR::is_a<ASR::Const_t>(*type) ) {
            storage_type = ASR::storage_typeType::Parameter;
        }
        ASR::abiType current_procedure_abi_type = abi;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type);
        ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(),
                s_intent, nullptr, nullptr, storage_type, type,
                nullptr,
                current_procedure_abi_type, s_access, s_presence,
                value_attr);
        ASR::symbol_t* v_sym = ASR::down_cast<ASR::symbol_t>(v);
        current_scope->add_or_overwrite_symbol(var_name, v_sym);
    }

    ASR::expr_t* fill_shape_and_lower_bound_for_CPtrToPointer(ASR::ttype_t* target_type,
                ASR::ttype_t* asr_alloc_type,
                ASR::expr_t* target_shape, const Location& loc) {
        ASR::dimension_t* target_dims = nullptr;
        int target_n_dims = ASRUtils::extract_dimensions_from_ttype(target_type, target_dims);
        if( target_n_dims <= 0 ) {
            return nullptr;
        }
        ASR::dimension_t* alloc_asr_type_dims = nullptr;
        int alloc_asr_type_n_dims = ASRUtils::extract_dimensions_from_ttype(
            asr_alloc_type, alloc_asr_type_dims);
        for( int i = 0; i < alloc_asr_type_n_dims; i++ ) {
            if( alloc_asr_type_dims[i].m_length != nullptr ||
                alloc_asr_type_dims[i].m_start != nullptr ) {
                throw SemanticError("Target type specified in "
                    "c_p_pointer must have deferred shape.",
                    loc);
            }
        }
        if( target_shape == nullptr ) {
            throw SemanticError("shape argument not specified in c_f_pointer "
                                "even though pptr is an array.",
                                loc);
        }
        int shape_rank = ASRUtils::extract_n_dims_from_ttype(
            ASRUtils::expr_type(target_shape));
        if( shape_rank != 1 ) {
            throw SemanticError("shape array passed to c_p_pointer "
                                "must be of rank 1 but given rank is " +
                                std::to_string(shape_rank), loc);
        }
        Vec<ASR::expr_t*> lbs;
        lbs.reserve(al, target_n_dims);
        for( int i = 0; i < target_n_dims; i++ ) {
            lbs.push_back(al, ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                al, loc, 0, ASRUtils::TYPE(
                    ASR::make_Integer_t(al, loc, 4)))));
        }
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 1);
        ASR::dimension_t dim;
        dim.loc = loc;
        dim.m_length = make_ConstantWithKind(make_IntegerConstant_t,
            make_Integer_t, target_n_dims, 4, loc);
        dim.m_start = make_ConstantWithKind(make_IntegerConstant_t,
            make_Integer_t, 0, 4, loc);
        dims.push_back(al, dim);
        ASR::ttype_t* type = ASRUtils::make_Array_t_util(al, loc,
            ASRUtils::expr_type(lbs[0]), dims.p, dims.size(), ASR::abiType::Source,
            false, ASR::array_physical_typeType::PointerToDataArray, true);
        return ASRUtils::EXPR(ASR::make_ArrayConstant_t(al,
            loc, lbs.p, lbs.size(), type,
            ASR::arraystorageType::RowMajor));
    }

    ASR::asr_t* create_CPtrToPointer(const AST::Call_t& x) {
        if( x.n_args != 2 && x.n_args != 3 ) {
            throw SemanticError("c_p_pointer accepts maximum three positional arguments, "
                                "first a variable of c_ptr type, second "
                                "the target type of the first variable and "
                                "third optionally the shape of the target variable "
                                "if target variable is an array",
                                x.base.base.loc);
        }
        this->visit_expr(*x.m_args[0]);
        ASR::expr_t* cptr = ASRUtils::EXPR(tmp);
        ASR::expr_t* pptr = assign_asr_target;
        ASR::expr_t* target_shape = nullptr;
        if( x.n_args == 3 ) {
            this->visit_expr(*x.m_args[2]);
            target_shape = ASRUtils::EXPR(tmp);
        }
        bool is_allocatable = false;
        ASR::ttype_t* asr_alloc_type = ast_expr_to_asr_type(x.m_args[1]->base.loc, *x.m_args[1], is_allocatable);
        ASR::ttype_t* target_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(pptr));
        if( !ASRUtils::types_equal(target_type, asr_alloc_type, true) ) {
            diag.add(diag::Diagnostic(
                "Type mismatch in c_p_pointer and target variable, the types must match",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + ASRUtils::type_to_str_python(target_type) +
                                "' and '" + ASRUtils::type_to_str_python(asr_alloc_type) + "')",
                            {target_type->base.loc, asr_alloc_type->base.loc})
                })
            );
            throw SemanticAbort();
        }
        if (ASR::is_a<ASR::Struct_t>(*asr_alloc_type)) {
            ASR::symbol_t *sym = ASRUtils::symbol_get_past_external(ASR::down_cast<ASR::Struct_t>(asr_alloc_type)->m_derived_type);
            if (ASR::is_a<ASR::StructType_t>(*sym)) {
                ASR::StructType_t *st = ASR::down_cast<ASR::StructType_t>(sym);
                if (st->m_abi != ASR::abiType::BindC) {
                    diag.add(diag::Diagnostic(
                        "The struct in c_p_pointer must be C interoperable",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("not C interoperable",
                                    {asr_alloc_type->base.loc}),
                            diag::Label("help: add the @ccallable decorator to this struct to make it C interoperable",
                                    {st->base.base.loc}, false)
                        })
                    );
                    throw SemanticAbort();
                }
            }
        }
        const Location& loc = x.base.base.loc;
        ASR::expr_t* lower_bounds = fill_shape_and_lower_bound_for_CPtrToPointer(
            target_type, asr_alloc_type,
            target_shape, loc);
        return ASR::make_CPtrToPointer_t(al, loc, cptr,
            pptr, target_shape, lower_bounds);
    }

    void handle_lambda_function_declaration(std::string &var_name, ASR::FunctionType_t* fn_type, AST::expr_t* value, const Location &loc) {
        if (value == nullptr) {
            throw SemanticError("Callback functions must have a value", loc);
        }

        if (!AST::is_a<AST::Lambda_t>(*value)) {
            throw SemanticError("Callback functions supports only lambda expressions as value", value->base.loc);
        }

        const AST::Lambda_t &x = *AST::down_cast<AST::Lambda_t>(value);
        if (fn_type->n_arg_types != x.m_args.n_args) {
            diag.add(diag::Diagnostic(
                "The number of args to lambda function much match the number of args declared in function type",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("",
                            {fn_type->base.base.loc, x.m_args.loc})
                })
            );
            throw SemanticAbort();
        }

        // Add the lambda function to the current scope
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);

        Vec<ASR::expr_t*> args;
        args.reserve(al, fn_type->n_arg_types);
        for (size_t i=0; i<fn_type->n_arg_types; i++) {
            std::string arg_name = x.m_args.m_args[i].m_arg;
            ASR::symbol_t *v;
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec,
                    fn_type->m_arg_types[i]);
            v = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, x.m_args.m_args[i].loc,
                current_scope, s2c(al, arg_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(), ASRUtils::intent_unspecified,
                nullptr, nullptr, ASR::storage_typeType::Default, fn_type->m_arg_types[i],
                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required,
                false));
            current_scope->add_symbol(arg_name, v);
            LCOMPILERS_ASSERT(v != nullptr)
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.m_args.m_args[i].loc, v)));
        }

        this->visit_expr(*x.m_body);
        ASR::asr_t* return_var_assign_stmt = make_dummy_assignment(ASRUtils::EXPR(tmp));
        ASR::expr_t *return_var = ASR::down_cast2<ASR::Assignment_t>(return_var_assign_stmt)->m_target;

        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(return_var), fn_type->m_return_var_type)) {
            std::string ltype = ASRUtils::type_to_str_python(ASRUtils::expr_type(return_var));
            std::string rtype = ASRUtils::type_to_str_python(fn_type->m_return_var_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in lambda expression return value",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                            {ASRUtils::expr_type(return_var)->base.loc, fn_type->m_return_var_type->base.loc})
                })
            );
            throw SemanticAbort();
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, 0);
        body.push_back(al, ASRUtils::STMT(return_var_assign_stmt));

        ASR::asr_t* fn_sym_util = ASRUtils::make_Function_t_util(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ s2c(al, var_name),
            nullptr, 0,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ body.p,
            /* n_body */ body.size(),
            /* a_return_var */ return_var,
            ASR::abiType::BindC, ASR::accessType::Public, ASR::deftypeType::Implementation,
            nullptr, false, false, false, false, false, nullptr, 0, false, false, false);
        current_scope = parent_scope;
        ASR::symbol_t* fn_sym = ASR::down_cast<ASR::symbol_t>(fn_sym_util);
        current_scope->add_symbol(var_name, fn_sym);
        tmp = nullptr;
    }

    void visit_AnnAssignUtil(const AST::AnnAssign_t& x, std::string& var_name,
                             ASR::expr_t* &init_expr,
                             bool wrap_derived_type_in_pointer=false,
                             ASR::abiType abi=ASR::abiType::Source,
                             bool inside_struct=false) {
        bool is_allocatable = false;
        ASR::ttype_t *type = nullptr;
        if( inside_struct ) {
            type = ast_expr_to_asr_type(x.m_annotation->base.loc, *x.m_annotation, is_allocatable, true);
        } else {
            type = ast_expr_to_asr_type(x.m_annotation->base.loc, *x.m_annotation, is_allocatable, true, abi);
        }
        if (ASR::is_a<ASR::FunctionType_t>(*type)) {
            ASR::FunctionType_t* fn_type = ASR::down_cast<ASR::FunctionType_t>(type);
            handle_lambda_function_declaration(var_name, fn_type, x.m_value, x.base.base.loc);
            return;
        }
        if( ASR::is_a<ASR::Struct_t>(*type) &&
            wrap_derived_type_in_pointer ) {
            type = ASRUtils::TYPE(ASR::make_Pointer_t(al, type->base.loc, type));
        }
        ASR::storage_typeType storage_type = ASR::storage_typeType::Default;
        if (is_allocatable) {
            type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, type->base.loc,
                ASRUtils::type_get_past_pointer(type)));
        }

        create_add_variable_to_scope(var_name, type,
                x.base.base.loc, abi, storage_type);

        ASR::expr_t* assign_asr_target_copy = assign_asr_target;
        this->visit_expr(*x.m_target);
        assign_asr_target = ASRUtils::EXPR(tmp);

        if( !init_expr ) {
            tmp = nullptr;
            if (x.m_value) {
                this->visit_expr(*x.m_value);
            } else {
                if (ASR::is_a<ASR::Struct_t>(*type)) {
                    //`s` must be initialized with an instance of S
                    throw  SemanticError("`" + var_name + "` must be initialized with an instance of " +
                            ASRUtils::type_to_str_python(type), x.base.base.loc);
                }
            }
            if (tmp && ASR::is_a<ASR::expr_t>(*tmp)) {
                ASR::expr_t* value = ASRUtils::EXPR(tmp);
                ASR::ttype_t* underlying_type = type;
                if( ASR::is_a<ASR::Const_t>(*type) ) {
                    underlying_type = ASRUtils::get_contained_type(type);
                }
                cast_helper(underlying_type, value, value->base.loc);
                if (!ASRUtils::check_equal_type(underlying_type, ASRUtils::expr_type(value), true)) {
                    std::string ltype = ASRUtils::type_to_str_python(underlying_type);
                    std::string rtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(value));
                    diag.add(diag::Diagnostic(
                        "Type mismatch in annotation-assignment, the types must be compatible",
                        diag::Level::Error, diag::Stage::Semantic, {
                            diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                                    {x.m_target->base.loc, value->base.loc})
                        })
                    );
                    throw SemanticAbort();
                }
                init_expr = value;
            }
        } else {
            cast_helper(type, init_expr, init_expr->base.loc);
        }

        if (!inside_struct || ASR::is_a<ASR::Const_t>(*type)) {
            process_variable_init_val(current_scope->get_symbol(var_name), x.base.base.loc, init_expr);
        }

        if ( !(tmp && ASR::is_a<ASR::stmt_t>(*tmp) &&
            (ASR::is_a<ASR::CPtrToPointer_t>(*ASR::down_cast<ASR::stmt_t>(tmp)) ||
             ASR::is_a<ASR::Allocate_t>(*ASR::down_cast<ASR::stmt_t>(tmp)))) ) {
            tmp = nullptr;
        }
        assign_asr_target = assign_asr_target_copy;
    }

    void visit_ClassMembers(const AST::ClassDef_t& x,
        Vec<char*>& member_names, SetChar& struct_dependencies,
        Vec<ASR::call_arg_t> &member_init,
        bool is_enum_scope=false, ASR::abiType abi=ASR::abiType::Source) {
        int64_t prev_value = 1;
        for( size_t i = 0; i < x.n_body; i++ ) {
            if (AST::is_a<AST::Expr_t>(*x.m_body[i])) {
                AST::Expr_t* expr = AST::down_cast<AST::Expr_t>(x.m_body[i]);
                if (AST::is_a<AST::ConstantStr_t>(*expr->m_value)) {
                    // It is a doc string. Skip doc strings for now.
                    continue;
                } else if (AST::is_a<AST::ConstantEllipsis_t>(*expr->m_value)) {
                    continue;
                }
                throw SemanticError("Only doc strings and const ellipsis allowed as expressions inside class", expr->base.base.loc);
            } else if( AST::is_a<AST::ClassDef_t>(*x.m_body[i]) ) {
                visit_ClassDef(*AST::down_cast<AST::ClassDef_t>(x.m_body[i]));
                continue;
            } else if ( AST::is_a<AST::FunctionDef_t>(*x.m_body[i]) ) {
                throw SemanticError("Struct member functions are not supported", x.m_body[i]->base.loc);
            } else if (AST::is_a<AST::Pass_t>(*x.m_body[i])) {
                continue;
            } else if (!AST::is_a<AST::AnnAssign_t>(*x.m_body[i])) {
                throw SemanticError("AnnAssign expected inside struct", x.m_body[i]->base.loc);
            }
            AST::AnnAssign_t* ann_assign = AST::down_cast<AST::AnnAssign_t>(x.m_body[i]);
            if (!AST::is_a<AST::Name_t>(*ann_assign->m_target)) {
                throw SemanticError("Only Name supported as target in AnnAssign inside struct", x.m_body[i]->base.loc);
            }
            AST::Name_t *n = AST::down_cast<AST::Name_t>(ann_assign->m_target);
            std::string var_name = n->m_id;
            ASR::expr_t* init_expr = nullptr;
            if( is_enum_scope ) {
                ASR::ttype_t* i64_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                init_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, -1, i64_type));
            }
            visit_AnnAssignUtil(*ann_assign, var_name, init_expr, false, abi, true);
            ASR::symbol_t* var_sym = current_scope->resolve_symbol(var_name);
            ASR::call_arg_t c_arg;
            c_arg.loc = var_sym->base.loc;
            c_arg.m_value = init_expr;
            member_init.push_back(al, c_arg);
            if( is_enum_scope ) {
                if( AST::is_a<AST::Call_t>(*ann_assign->m_value) ) {
                    AST::Call_t* auto_call_cand = AST::down_cast<AST::Call_t>(ann_assign->m_value);
                    if( AST::is_a<AST::Name_t>(*auto_call_cand->m_func) ) {
                        AST::Name_t* func = AST::down_cast<AST::Name_t>(auto_call_cand->m_func);
                        std::string func_name = func->m_id;
                        if( func_name == "auto" ) {
                            ASR::ttype_t* int_type = ASRUtils::symbol_type(var_sym);
                            init_expr = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al,
                                auto_call_cand->base.base.loc, prev_value, int_type));
                            prev_value += 1;
                        }
                    }
                } else {
                    this->visit_expr(*ann_assign->m_value);
                    ASR::expr_t* enum_value = ASRUtils::expr_value(ASRUtils::EXPR(tmp));
                    LCOMPILERS_ASSERT(ASRUtils::is_value_constant(enum_value));
                    ASRUtils::extract_value(enum_value, prev_value);
                    prev_value += 1;
                    init_expr = enum_value;
                }
            } else {
                init_expr = nullptr;
            }
            if( ASR::is_a<ASR::Variable_t>(*var_sym) ) {
                ASR::Variable_t* variable = ASR::down_cast<ASR::Variable_t>(var_sym);
                variable->m_symbolic_value = init_expr;
            }
            ASR::ttype_t* var_type = ASRUtils::type_get_past_pointer(ASRUtils::symbol_type(var_sym));
            char* aggregate_type_name = nullptr;
            if( ASR::is_a<ASR::Struct_t>(*var_type) ) {
                aggregate_type_name = ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Struct_t>(var_type)->m_derived_type);
            } else if( ASR::is_a<ASR::Enum_t>(*var_type) ) {
                aggregate_type_name = ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Enum_t>(var_type)->m_enum_type);
            } else if( ASR::is_a<ASR::Union_t>(*var_type) ) {
                aggregate_type_name = ASRUtils::symbol_name(
                    ASR::down_cast<ASR::Union_t>(var_type)->m_union_type);
            }
            if( aggregate_type_name &&
                !current_scope->get_symbol(std::string(aggregate_type_name)) ) {
                struct_dependencies.push_back(al, aggregate_type_name);
            }
            member_names.push_back(al, n->m_id);
        }
    }

    ASR::abiType get_abi_from_decorators(AST::expr_t** decorators, size_t n) {
        for( size_t i = 0; i < n; i++ ) {
            AST::expr_t* decorator = decorators[i];
            if( !AST::is_a<AST::Name_t>(*decorator) ) {
                continue;
            }

            AST::Name_t* dec_name = AST::down_cast<AST::Name_t>(decorator);
            if( std::string(dec_name->m_id) == "ccall" ) {
                return ASR::abiType::BindC;
            }
        }
        return ASR::abiType::Source;
    }

    void visit_ClassDef(const AST::ClassDef_t& x) {
        std::string x_m_name = x.m_name;
        if( is_enum(x.m_bases, x.n_bases) ) {
            if( current_scope->resolve_symbol(x_m_name) ) {
                return ;
            }
            ASR::abiType enum_abi = get_abi_from_decorators(x.m_decorator_list, x.n_decorator_list);
            SymbolTable *parent_scope = current_scope;
            current_scope = al.make_new<SymbolTable>(parent_scope);
            Vec<char*> member_names;
            Vec<ASR::call_arg_t>  member_init;
            member_names.reserve(al, x.n_body);
            member_init.reserve(al, 1);
            Vec<ASR::stmt_t*>* current_body_copy = current_body;
            current_body = nullptr;
            SetChar struct_dependencies;
            struct_dependencies.reserve(al, 1);
            visit_ClassMembers(x, member_names, struct_dependencies, member_init, true, enum_abi);
            current_body = current_body_copy;
            ASR::ttype_t* common_type = nullptr;
            for( auto sym: current_scope->get_scope() ) {
                ASR::Variable_t* enum_mem_var = ASR::down_cast<ASR::Variable_t>(sym.second);
                if( common_type == nullptr ) {
                    common_type = enum_mem_var->m_type;
                } else {
                    if( !ASRUtils::check_equal_type(common_type, enum_mem_var->m_type) ) {
                        throw SemanticError("All members of enum should be of the same type.", x.base.base.loc);
                    }
                }
            }
            ASR::enumtypeType enum_value_type = ASR::enumtypeType::NonInteger;
            if( ASR::is_a<ASR::Integer_t>(*common_type) ) {
                int8_t IntegerConsecutiveFromZero = 1;
                int8_t IntegerNotUnique = 0;
                int8_t IntegerUnique = 1;
                std::map<int64_t, int64_t> value2count;
                for( auto sym: current_scope->get_scope() ) {
                    ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(sym.second);
                    ASR::expr_t* value = ASRUtils::expr_value(member_var->m_symbolic_value);
                    int64_t value_int64 = -1;
                    ASRUtils::extract_value(value, value_int64);
                    if( value2count.find(value_int64) == value2count.end() ) {
                        value2count[value_int64] = 0;
                    }
                    value2count[value_int64] += 1;
                }
                int64_t prev = -1;
                for( auto itr: value2count ) {
                    if( itr.second > 1 ) {
                        IntegerNotUnique = 1;
                        IntegerUnique = 0;
                        IntegerConsecutiveFromZero = 0;
                        break ;
                    }
                    if( itr.first - prev != 1 ) {
                        IntegerConsecutiveFromZero = 0;
                    }
                    prev = itr.first;
                }
                if( IntegerConsecutiveFromZero ) {
                    if( value2count.find(0) == value2count.end() ) {
                        IntegerConsecutiveFromZero = 0;
                        IntegerUnique = 1;
                    } else {
                        IntegerUnique = 0;
                    }
                }
                LCOMPILERS_ASSERT(IntegerConsecutiveFromZero + IntegerNotUnique + IntegerUnique == 1);
                if( IntegerConsecutiveFromZero ) {
                    enum_value_type = ASR::enumtypeType::IntegerConsecutiveFromZero;
                } else if( IntegerNotUnique ) {
                    enum_value_type = ASR::enumtypeType::IntegerNotUnique;
                } else if( IntegerUnique ) {
                    enum_value_type = ASR::enumtypeType::IntegerUnique;
                }
            }
            if( (enum_value_type == ASR::enumtypeType::NonInteger ||
                 enum_value_type == ASR::enumtypeType::IntegerNotUnique) &&
                 enum_abi == ASR::abiType::BindC ) {
                throw SemanticError("Enumerations with non-integer or non-unique integer "
                                    "values cannot interoperate with C code.",
                                    x.base.base.loc);
            }
            ASR::symbol_t* enum_type = ASR::down_cast<ASR::symbol_t>(ASR::make_EnumType_t(al,
                                            x.base.base.loc, current_scope, x.m_name,
                                            struct_dependencies.p, struct_dependencies.size(),
                                            member_names.p, member_names.size(),
                                            enum_abi, ASR::accessType::Public, enum_value_type,
                                            common_type, nullptr));
            current_scope = parent_scope;
            current_scope->add_symbol(std::string(x.m_name), enum_type);
            return ;
        } else if( is_union(x.m_bases, x.n_bases) ) {
            SymbolTable *parent_scope = current_scope;
            current_scope = al.make_new<SymbolTable>(parent_scope);
            Vec<char*> member_names;
            Vec<ASR::call_arg_t> member_init;
            member_names.reserve(al, x.n_body);
            member_init.reserve(al, x.n_body);
            SetChar struct_dependencies;
            struct_dependencies.reserve(al, 1);
            visit_ClassMembers(x, member_names, struct_dependencies, member_init);
            LCOMPILERS_ASSERT(member_init.size() == member_names.size());
            ASR::symbol_t* union_type = ASR::down_cast<ASR::symbol_t>(ASR::make_UnionType_t(al,
                                            x.base.base.loc, current_scope, x.m_name,
                                            struct_dependencies.p, struct_dependencies.size(),
                                            member_names.p, member_names.size(),
                                            ASR::abiType::Source, ASR::accessType::Public,
                                            member_init.p, member_init.size(), nullptr));
            current_scope = parent_scope;
            if (current_scope->resolve_symbol(x_m_name)) {
                ASR::symbol_t* sym = current_scope->resolve_symbol(x_m_name);
                ASR::UnionType_t *ut = ASR::down_cast<ASR::UnionType_t>(sym);
                ut->m_initializers = member_init.p;
                ut->n_initializers = member_init.size();
            } else {
                current_scope->add_symbol(x_m_name, union_type);
            }
            return ;
        }
        ASR::expr_t* algined_expr = nullptr;
        bool is_packed = false;
        if( !is_dataclass(x.m_decorator_list, x.n_decorator_list,
                          algined_expr, is_packed) ) {
            throw SemanticError("Only dataclass-decorated classes and Enum subclasses are supported.",
                                x.base.base.loc);
        }

        if( x.n_bases > 0 ) {
            throw SemanticError("Inheritance in classes isn't supported yet.",
                                x.base.base.loc);
        }

        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        Vec<char*> member_names;
        Vec<ASR::call_arg_t> member_init;
        member_names.reserve(al, x.n_body);
        member_init.reserve(al, x.n_body);
        SetChar struct_dependencies;
        struct_dependencies.reserve(al, 1);
        ASR::abiType class_abi = ASR::abiType::Source;
        if( is_bindc_class(x.m_decorator_list, x.n_decorator_list) ) {
            class_abi = ASR::abiType::BindC;
        }
        visit_ClassMembers(x, member_names, struct_dependencies, member_init, false, class_abi);
        LCOMPILERS_ASSERT(member_init.size() == member_names.size());
        ASR::symbol_t* class_type = ASR::down_cast<ASR::symbol_t>(ASR::make_StructType_t(al,
                                        x.base.base.loc, current_scope, x.m_name,
                                        struct_dependencies.p, struct_dependencies.size(),
                                        member_names.p, member_names.size(),
                                        class_abi, ASR::accessType::Public,
                                        is_packed, false, member_init.p, member_init.size(), algined_expr,
                                        nullptr));
        current_scope = parent_scope;
        if (current_scope->resolve_symbol(x_m_name)) {
            ASR::symbol_t* sym = current_scope->resolve_symbol(x_m_name);
            ASR::StructType_t *st = ASR::down_cast<ASR::StructType_t>(sym);
            st->m_initializers = member_init.p;
            st->n_initializers = member_init.size();
        } else {
            current_scope->add_symbol(x_m_name, class_type);
        }
    }

    void add_name(const Location &loc) {
        std::string var_name = "__name__";
        std::string var_value = module_name;
        size_t s_size = var_value.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, s_size, nullptr));
        ASR::expr_t *value = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
            loc, s2c(al, var_value), type));
        ASR::expr_t *init_expr = value;

        ASR::intentType s_intent = ASRUtils::intent_local;
        ASR::storage_typeType storage_type =
                ASR::storage_typeType::Default;
        if( ASR::is_a<ASR::Const_t>(*type) ) {
            storage_type = ASR::storage_typeType::Parameter;
        }
        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type, init_expr, value);
        ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(), s_intent, init_expr,
                value, storage_type, type, nullptr, current_procedure_abi_type,
                s_access, s_presence, value_attr);
        current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));
    }

    void add_lpython_version(const Location &loc) {
        std::string var_name = "__LPYTHON_VERSION__";
        std::string var_value = LFORTRAN_VERSION;
        size_t s_size = var_value.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, s_size, nullptr));
        ASR::expr_t *value = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
            loc, s2c(al, var_value), type));
        ASR::expr_t *init_expr = value;

        ASR::intentType s_intent = ASRUtils::intent_local;
        ASR::storage_typeType storage_type =
                ASR::storage_typeType::Default;
        if( ASR::is_a<ASR::Const_t>(*type) ) {
            storage_type = ASR::storage_typeType::Parameter;
        }
        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type, init_expr, value);
        ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(),
                s_intent, init_expr, value, storage_type, type, nullptr,
                current_procedure_abi_type, s_access, s_presence,
                value_attr);
        current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));
    }

    void visit_Name(const AST::Name_t &x) {
        std::string name = x.m_id;
        ASR::symbol_t *s = current_scope->resolve_symbol(name);
        std::set<std::string> not_cpython_builtin = {
            "pi", "E"};
        if (s) {
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else if (name == "i32" || name == "i64" || name == "f32" ||
                   name == "f64" || name == "c32" || name == "c64" ||
                   name == "i16" || name == "i8") {
            Vec<ASR::dimension_t> dims;
            dims.reserve(al, 1);
            ASR::ttype_t *type = get_type_from_var_annotation(name,
                                    x.base.base.loc, dims, nullptr, 0);
            tmp = (ASR::asr_t*) ASRUtils::get_constant_zero_with_given_type(al, type);
        } else if (name == "__name__") {
            // __name__ was not declared yet in this scope, so we
            // declare it first:
            add_name(x.base.base.loc);
            // And now resolve it:
            ASR::symbol_t *s = current_scope->resolve_symbol(name);
            LCOMPILERS_ASSERT(s);
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else if (name == "__LPYTHON_VERSION__") {
            add_lpython_version(x.base.base.loc);
            ASR::symbol_t *s = current_scope->resolve_symbol(name);
            LCOMPILERS_ASSERT(s);
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else if (ASRUtils::IntrinsicScalarFunctionRegistry::is_intrinsic_function(name) &&
                   (not_cpython_builtin.find(name) == not_cpython_builtin.end() ||
                   imported_functions.find(name) != imported_functions.end() )) {
                    ASRUtils::create_intrinsic_function create_func =
                        ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function(name);
                    Vec<ASR::expr_t*> args_;
                    tmp = create_func(al, x.base.base.loc, args_, [&](const std::string &msg, const Location &loc) {
                    throw SemanticError(msg, loc); });
        } else {
            throw SemanticError("Variable '" + name + "' not declared",
                x.base.base.loc);
        }
    }

    void visit_NamedExpr(const AST::NamedExpr_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = ASRUtils::EXPR(tmp);
        [[maybe_unused]] ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
        LCOMPILERS_ASSERT(ASRUtils::check_equal_type(target_type, value_type));
        tmp = ASR::make_NamedExpr_t(al, x.base.base.loc, target, value, value_type);
    }

    void visit_Tuple(const AST::Tuple_t &x) {
        Vec<ASR::expr_t*> elements;
        elements.reserve(al, x.n_elts);
        Vec<ASR::ttype_t*> tuple_type_vec;
        tuple_type_vec.reserve(al, x.n_elts);
        for (size_t i=0; i<x.n_elts; i++) {
            this->visit_expr(*x.m_elts[i]);
            ASR::expr_t *expr = ASRUtils::EXPR(tmp);
            elements.push_back(al, expr);
            tuple_type_vec.push_back(al, ASRUtils::expr_type(expr));
        }
        ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, x.base.base.loc,
                                    tuple_type_vec.p, tuple_type_vec.n));
        tmp = ASR::make_TupleConstant_t(al, x.base.base.loc,
                                    elements.p, elements.size(), tuple_type);
    }

    void visit_ConstantInt(const AST::ConstantInt_t &x) {
        int64_t i = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
        tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, i, type);
    }

    void visit_ConstantFloat(const AST::ConstantFloat_t &x) {
        double f = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, 8));
        tmp = ASR::make_RealConstant_t(al, x.base.base.loc, f, type);
    }

    void visit_ConstantComplex(const AST::ConstantComplex_t &x) {
        double re = x.m_re, im = x.m_im;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, 8));
        tmp = ASR::make_ComplexConstant_t(al, x.base.base.loc, re, im, type);
    }

    void visit_ConstantStr(const AST::ConstantStr_t &x) {
        char *s = x.m_value;
        size_t s_size = std::string(s).size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                1, s_size, nullptr));
        tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s, type);
    }

    void visit_ConstantBool(const AST::ConstantBool_t &x) {
        bool b = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4));
        tmp = ASR::make_LogicalConstant_t(al, x.base.base.loc, b, type);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        ASR::logicalbinopType op;
        if (x.n_values > 2) {
            throw SemanticError("Only two operands supported for boolean operations",
                x.base.base.loc);
        }
        this->visit_expr(*x.m_values[0]);
        ASR::expr_t *lhs = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_values[1]);
        ASR::expr_t *rhs = ASRUtils::EXPR(tmp);
        switch (x.m_op) {
            case (AST::boolopType::And): { op = ASR::logicalbinopType::And; break; }
            case (AST::boolopType::Or): { op = ASR::logicalbinopType::Or; break; }
            default : {
                throw SemanticError("Boolean operator type not supported",
                    x.base.base.loc);
            }
        }
        LCOMPILERS_ASSERT(
            ASRUtils::check_equal_type(ASRUtils::expr_type(lhs), ASRUtils::expr_type(rhs)));
        ASR::expr_t *value = nullptr;
        ASR::ttype_t *dest_type = ASRUtils::expr_type(lhs);

        if (ASRUtils::expr_value(lhs) != nullptr && ASRUtils::expr_value(rhs) != nullptr) {

            LCOMPILERS_ASSERT(ASR::is_a<ASR::Logical_t>(*dest_type));
            bool left_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                    ASRUtils::expr_value(lhs))->m_value;
            bool right_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                    ASRUtils::expr_value(rhs))->m_value;
            bool result;
            switch (op) {
                case (ASR::logicalbinopType::And): { result = left_value && right_value; break; }
                case (ASR::logicalbinopType::Or): { result = left_value || right_value; break; }
                default : {
                    throw SemanticError("Boolean operator type not supported",
                        x.base.base.loc);
                }
            }
            value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                al, x.base.base.loc, result, dest_type));
        }
        tmp = ASR::make_LogicalBinOp_t(al, x.base.base.loc, lhs, op, rhs, dest_type, value);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);
        ASR::binopType op = ASR::binopType::Add /* temporary assignment */;
        std::string op_name = "";
        switch (x.m_op) {
            case (AST::operatorType::Add) : { op = ASR::binopType::Add; break; }
            case (AST::operatorType::Sub) : { op = ASR::binopType::Sub; break; }
            case (AST::operatorType::Mult) : { op = ASR::binopType::Mul; break; }
            case (AST::operatorType::Div) : { op = ASR::binopType::Div; break; }
            case (AST::operatorType::FloorDiv) : {op_name = "floordiv"; break;}
            case (AST::operatorType::Pow) : { op = ASR::binopType::Pow; break; }
            case (AST::operatorType::BitOr) : { op = ASR::binopType::BitOr; break; }
            case (AST::operatorType::BitAnd) : { op = ASR::binopType::BitAnd; break; }
            case (AST::operatorType::BitXor) : { op = ASR::binopType::BitXor; break; }
            case (AST::operatorType::LShift) : { op = ASR::binopType::BitLShift; break; }
            case (AST::operatorType::RShift) : { op = ASR::binopType::BitRShift; break; }
            case (AST::operatorType::Mod) : { op_name = "_mod"; break; }
            default : {
                throw SemanticError("Binary operator type not supported",
                    x.base.base.loc);
            }
        }

        cast_helper(left, right, false);

        if (op_name == "floordiv") {
            Vec<ASR::expr_t*> args;
            args.reserve(al, 2);
            args.push_back(al, left);
            args.push_back(al, right);
            ASRUtils::create_intrinsic_function create_func =
                        ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function(op_name);
            tmp = create_func(al, x.base.base.loc, args,
                            [&](const std::string &msg, const Location &loc) {
                                throw SemanticError(msg, loc); });
            return;
        }

        if (op_name == "_mod") {
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 2);
            ASR::call_arg_t arg1, arg2;
            arg1.loc = left->base.loc;
            arg1.m_value = left;
            args.push_back(al, arg1);
            arg2.loc = right->base.loc;
            arg2.m_value = right;
            args.push_back(al, arg2);
            ASR::symbol_t *fn_mod = resolve_intrinsic_function(x.base.base.loc, op_name);
            tmp = make_call_helper(al, fn_mod, current_scope, args, op_name, x.base.base.loc);
            return;
        }
        make_BinOp_helper(left, right, op, x.base.base.loc);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = ASRUtils::EXPR(tmp);
        ASR::ttype_t *operand_type = ASRUtils::expr_type(operand);
        ASR::ttype_t *dest_type = operand_type;
        ASR::ttype_t *logical_type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4));
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
        ASR::expr_t *value = nullptr;

        if (x.m_op == AST::unaryopType::Invert) {
            if (ASRUtils::is_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                        al, x.base.base.loc, ~op_value, operand_type));
                }
                tmp = ASR::make_IntegerBitNot_t(al, x.base.base.loc, operand, dest_type, value);
                return;
            } else if (ASRUtils::is_unsigned_integer(*operand_type)) {
                int kind = ASRUtils::extract_kind_from_ttype_t(operand_type);
                int signed_promote_kind = (kind < 8) ? kind * 2 : kind;
                diag.add(diag::Diagnostic(
                    "The result of the bitnot ~ operation is negative, thus out of range for u" + std::to_string(kind * 8),
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("use ~i" + std::to_string(signed_promote_kind * 8)
                            + "(u) for signed result or bitnot_u" + std::to_string(kind * 8) + "(u) for unsigned result",
                                {x.base.base.loc})
                    })
                );
                throw SemanticAbort();
            } else if (ASRUtils::is_real(*operand_type)) {
                throw SemanticError("Unary operator '~' not supported for floats",
                    x.base.base.loc);
            } else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                        al, x.base.base.loc, op_value ? -2 : -1, int_type));
                }
                // cast Logical to Integer
                // Reason: Resultant type of an unary operation should be the same as operand type
                ASR::expr_t *int_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                            al, x.base.base.loc, operand, ASR::cast_kindType::LogicalToInteger,
                            int_type, value));
                tmp = ASR::make_IntegerBitNot_t(al, x.base.base.loc, int_arg, int_type, value);
                return;
            } else if (ASRUtils::is_complex(*operand_type)) {
                throw SemanticError("Unary operator '~' not supported for complex type",
                    x.base.base.loc);
            } else {
                throw SemanticError("Unary operator '~' not supported for type " + ASRUtils::type_to_str_python(operand_type),
                    x.base.base.loc);
            }
        } else if (x.m_op == AST::unaryopType::Not) {
            ASR::expr_t *logical_arg = operand;
            if (ASRUtils::is_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(operand))->m_n;
                    bool b = (op_value == 0);
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                        al, x.base.base.loc, b, logical_type));
                }
                // cast Integer to Logical
                logical_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                            al, x.base.base.loc, operand, ASR::cast_kindType::IntegerToLogical,
                            logical_type, value));
            } else if (ASRUtils::is_unsigned_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                        ASRUtils::expr_value(operand))->m_n;
                    bool b = (op_value == 0);
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                        al, x.base.base.loc, b, logical_type));
                }
                // cast UnsignedInteger to Logical
                logical_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                            al, x.base.base.loc, operand, ASR::cast_kindType::UnsignedIntegerToLogical,
                            logical_type, value));
            } else if (ASRUtils::is_real(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    double op_value = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(operand))->m_r;
                    bool b = (op_value == 0.0);
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                        al, x.base.base.loc, b, logical_type));
                }
                // cast Real to Logical
                logical_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                            al, x.base.base.loc, operand, ASR::cast_kindType::RealToLogical,
                            logical_type, value));
            } else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                        al, x.base.base.loc, !op_value, logical_type));
                }
            } else if (ASRUtils::is_complex(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    if( ASR::is_a<ASR::FunctionCall_t>(*operand) ) {
                        ASR::FunctionCall_t* operand_func_call = ASR::down_cast<ASR::FunctionCall_t>(operand);
                        dependencies.erase(ASRUtils::symbol_name(operand_func_call->m_name));
                    }
                    ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(
                                        ASRUtils::expr_value(operand));
                    std::complex<double> op_value(c->m_re, c->m_im);
                    bool b = (op_value.real() == 0.0 && op_value.imag() == 0.0);
                    tmp = ASR::make_LogicalConstant_t(al, x.base.base.loc, b, logical_type);
                    return;
                }
                // cast Complex to Logical
                logical_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                            al, x.base.base.loc, operand, ASR::cast_kindType::ComplexToLogical,
                            logical_type, value));
            } else {
                throw SemanticError("Unary operator '!' not supported for type " + ASRUtils::type_to_str_python(operand_type),
                    x.base.base.loc);
            }

            tmp = ASR::make_LogicalNot_t(al, x.base.base.loc, logical_arg, logical_type, value);
            return;
        } else if (x.m_op == AST::unaryopType::UAdd) {
            if (ASRUtils::is_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, op_value, operand_type);
                }
            } else if (ASRUtils::is_unsigned_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    tmp = ASR::make_UnsignedIntegerConstant_t(al, x.base.base.loc, op_value, operand_type);
                }
            } else if (ASRUtils::is_real(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    double op_value = ASR::down_cast<ASR::RealConstant_t>(
                                            ASRUtils::expr_value(operand))->m_r;
                    tmp = ASR::make_RealConstant_t(al, x.base.base.loc, op_value, operand_type);
                }
            } else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, +op_value, int_type);
                    return;
                }
                tmp = ASR::make_Cast_t(al, x.base.base.loc, operand, ASR::cast_kindType::LogicalToInteger,
                        int_type, value);
            } else if (ASRUtils::is_complex(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    if( ASR::is_a<ASR::FunctionCall_t>(*operand) ) {
                        ASR::FunctionCall_t* operand_func_call = ASR::down_cast<ASR::FunctionCall_t>(operand);
                        dependencies.erase(ASRUtils::symbol_name(operand_func_call->m_name));
                    }
                    ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(
                                        ASRUtils::expr_value(operand));
                    std::complex<double> op_value(c->m_re, c->m_im);
                    tmp = ASR::make_ComplexConstant_t(al, x.base.base.loc,
                        std::real(op_value), std::imag(op_value), operand_type);
                }
            } else {
                throw SemanticError("Unary operator '+' not supported for type " + ASRUtils::type_to_str_python(operand_type),
                    x.base.base.loc);
            }
            return;
        } else if (x.m_op == AST::unaryopType::USub) {
            if (ASRUtils::is_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                        al, x.base.base.loc, -op_value, operand_type));
                }
                tmp = ASR::make_IntegerUnaryMinus_t(al, x.base.base.loc, operand,
                                                    operand_type, value);
                return;
            } else if (ASRUtils::is_unsigned_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    if (op_value != 0) {
                        int kind = ASRUtils::extract_kind_from_ttype_t(operand_type);
                        int signed_promote_kind = (kind < 8) ? kind * 2 : kind;
                        diag.add(diag::Diagnostic(
                            "The result of the unary minus `-` operation is negative, thus out of range for u" + std::to_string(kind * 8),
                            diag::Level::Error, diag::Stage::Semantic, {
                                diag::Label("use -i" + std::to_string(signed_promote_kind * 8)
                                    + "(u) for signed result", {x.base.base.loc})
                            })
                        );
                        throw SemanticAbort();
                    }
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_UnsignedIntegerConstant_t(
                        al, x.base.base.loc, 0, operand_type));
                }
                tmp = ASR::make_UnsignedIntegerUnaryMinus_t(al, x.base.base.loc, operand,
                                                    operand_type, value);
                return;
            } else if (ASRUtils::is_real(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    double op_value = ASR::down_cast<ASR::RealConstant_t>(
                                            ASRUtils::expr_value(operand))->m_r;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(
                        al, x.base.base.loc, -op_value, operand_type));
                }
                tmp = ASR::make_RealUnaryMinus_t(al, x.base.base.loc, operand,
                                                 operand_type, value);
                return;
            } else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                        al, x.base.base.loc, -op_value, int_type));
                }
                // cast Logical to Integer
                ASR::expr_t *int_arg = ASR::down_cast<ASR::expr_t>(ASR::make_Cast_t(
                        al, x.base.base.loc, operand, ASR::cast_kindType::LogicalToInteger,
                        int_type, value));
                tmp = ASR::make_IntegerUnaryMinus_t(al, x.base.base.loc, int_arg,
                                                    int_type, value);
                return;
            } else if (ASRUtils::is_complex(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(
                                        ASRUtils::expr_value(operand));
                    std::complex<double> op_value(c->m_re, c->m_im);
                    std::complex<double> result;
                    result = -op_value;
                    value = ASR::down_cast<ASR::expr_t>(
                        ASR::make_ComplexConstant_t(al, x.base.base.loc, std::real(result),
                        std::imag(result), operand_type));
                }
                tmp = ASR::make_ComplexUnaryMinus_t(al, x.base.base.loc, operand,
                                                    operand_type, value);
                return;
            } else {
                throw SemanticError("Unary operator '-' not supported for type " + ASRUtils::type_to_str_python(operand_type),
                    x.base.base.loc);
            }
        }
    }

    void visit_IfExp(const AST::IfExp_t &x) {
        this->visit_expr(*x.m_test);
        ASR::expr_t *test = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_body);
        ASR::expr_t *body = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_orelse);
        ASR::expr_t *orelse = ASRUtils::EXPR(tmp);
        LCOMPILERS_ASSERT(ASRUtils::check_equal_type(ASRUtils::expr_type(body),
                                                   ASRUtils::expr_type(orelse)));
        tmp = ASR::make_IfExp_t(al, x.base.base.loc, test, body, orelse,
                                ASRUtils::expr_type(body), nullptr);
    }

    bool visit_SubscriptIndices(AST::expr_t* m_slice, Vec<ASR::array_index_t>& args,
                                ASR::expr_t* value, ASR::ttype_t* type, bool& is_item,
                                const Location& loc) {
        ASR::array_index_t ai;
        ai.loc = loc;
        ai.m_left = nullptr;
        ai.m_right = nullptr;
        ai.m_step = nullptr;
        if (AST::is_a<AST::Slice_t>(*m_slice)) {
            AST::Slice_t *sl = AST::down_cast<AST::Slice_t>(m_slice);
            if (sl->m_lower != nullptr) {
                this->visit_expr(*sl->m_lower);
                if (!ASRUtils::is_integer(*ASRUtils::expr_type(ASRUtils::EXPR(tmp)))) {
                    throw SemanticError("slice indices must be integers or None", tmp->loc);
                }
                ai.m_left = ASRUtils::EXPR(tmp);
            }
            if (sl->m_upper != nullptr) {
                this->visit_expr(*sl->m_upper);
                if (!ASRUtils::is_integer(*ASRUtils::expr_type(ASRUtils::EXPR(tmp)))) {
                    throw SemanticError("slice indices must be integers or None", tmp->loc);
                }
                ai.m_right = ASRUtils::EXPR(tmp);
            }
            if (sl->m_step != nullptr) {
                this->visit_expr(*sl->m_step);
                ASR::asr_t *tmp_step = tmp;
                if (!ASRUtils::is_integer(*ASRUtils::expr_type(ASRUtils::EXPR(tmp)))) {
                    throw SemanticError("slice indices must be integers or None", tmp->loc);
                }
                ASR::expr_t* val = ASRUtils::expr_value(ASRUtils::EXPR(tmp_step));
                int64_t const_value = 1;
                if (ASRUtils::is_value_constant(val, const_value) && const_value == 0) {
                    throw SemanticError("slice step cannot be zero", tmp_step->loc);
                }
                ai.m_step = ASRUtils::EXPR(tmp);
            }
            if( ai.m_left != nullptr &&
                ASR::is_a<ASR::Var_t>(*ai.m_left) &&
                ASR::is_a<ASR::Var_t>(*ai.m_right) ) {
                ASR::Variable_t* startv = ASRUtils::EXPR2VAR(ai.m_left);
                ASR::Variable_t* endv = ASRUtils::EXPR2VAR(ai.m_right);
                is_item = is_item && (startv == endv);
            } else {
                is_item = is_item && (ai.m_left == nullptr &&
                                      ai.m_step == nullptr &&
                                      ai.m_right != nullptr);
            }
            if (ASR::is_a<ASR::List_t>(*type)) {
                tmp = ASR::make_ListSection_t(al, loc, value, ai, type, nullptr);
                return false;
            } else if (ASR::is_a<ASR::Character_t>(*type)) {
                tmp = ASR::make_StringSection_t(al, loc, value, ai.m_left, ai.m_right,
                    ai.m_step, type, nullptr);
                return false;
            } else if (ASR::is_a<ASR::Dict_t>(*type)) {
                throw SemanticError("unhashable type in dict: 'slice'", loc);
            }
        } else if(AST::is_a<AST::Tuple_t>(*m_slice) &&
                    ASRUtils::is_array(type)) {
            bool final_result = true;
            AST::Tuple_t* indices = AST::down_cast<AST::Tuple_t>(m_slice);
            for( size_t i = 0; i < indices->n_elts; i++ ) {
                final_result &= visit_SubscriptIndices(indices->m_elts[i], args,
                                                        value, type, is_item, loc);
            }
            return final_result;
        } else {
            this->visit_expr(*m_slice);
            if (!ASR::is_a<ASR::Dict_t>(*type) &&
                    !ASRUtils::is_integer(*ASRUtils::expr_type(ASRUtils::EXPR(tmp)))) {
                std::string fnd = ASRUtils::type_to_str_python(ASRUtils::expr_type(ASRUtils::EXPR(tmp)));
                diag.add(diag::Diagnostic(
                    "Type mismatch in index, expected a single integer or slice",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (found: '" + fnd + "', expected: 'i32' or slice)",
                                {tmp->loc})
                    })
                );
                throw SemanticAbort();
            }
            ASR::expr_t *index = nullptr;
            if (ASR::is_a<ASR::Dict_t>(*type)) {
                index = ASRUtils::EXPR(tmp);
                ASR::ttype_t *key_type = ASR::down_cast<ASR::Dict_t>(type)->m_key_type;
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(index), key_type)) {
                    throw SemanticError("Key type should be '" + ASRUtils::type_to_str_python(key_type) +
                                        "' instead of '" +
                                        ASRUtils::type_to_str_python(ASRUtils::expr_type(index)) + "'",
                            index->base.loc);
                }
                tmp = make_DictItem_t(al, loc, value, index, nullptr,
                                      ASR::down_cast<ASR::Dict_t>(type)->m_value_type, nullptr);
                return false;

            } else if (ASR::is_a<ASR::List_t>(*type)) {
                index = ASRUtils::EXPR(tmp);
                ASR::expr_t* val = ASRUtils::expr_value(index);
                if (val && ASR::is_a<ASR::IntegerConstant_t>(*val)) {
                    if (ASR::down_cast<ASR::IntegerConstant_t>(val)->m_n < 0) {
                        // Replace `x[-1]` to `x[len(x)+(-1)]`
                        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(
                                                        al, loc, 4));
                        ASR::expr_t *list_len = ASRUtils::EXPR(ASR::make_ListLen_t(
                                    al, loc, value, int_type, nullptr));
                        ASR::expr_t *neg_idx = ASRUtils::expr_value(index);
                        index = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                            list_len, ASR::binopType::Add, neg_idx, int_type, nullptr));
                    }
                }
                tmp = make_ListItem_t(al, loc, value, index,
                                      ASR::down_cast<ASR::List_t>(type)->m_type, nullptr);
                return false;
            } else if (ASR::is_a<ASR::Tuple_t>(*type)) {
                int i = 0;
                index = ASRUtils::EXPR(tmp);
                ASR::expr_t* val = ASRUtils::expr_value(index);
                if (!val) {
                    throw SemanticError("Runtime Indexing with " + ASRUtils::type_to_str_python(type) +
                    " is not possible.", loc);
                }

                if (ASR::is_a<ASR::IntegerConstant_t>(*val)) {
                    i = ASR::down_cast<ASR::IntegerConstant_t>(val)->m_n;
                    int tuple_size =  ASR::down_cast<ASR::Tuple_t>(type)->n_type;
                    if (i < 0) {
                        i = tuple_size + i;
                        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                        index = ASRUtils::EXPR(ASR::make_IntegerConstant_t(
                            al, loc, i, int_type));
                    }
                    if (i >= tuple_size || i < 0) {
                        throw SemanticError("Tuple index out of bounds", loc);
                    }
                } else {
                    throw SemanticError("Tuple indices must be constant integers", loc);
                }
                tmp = make_TupleItem_t(al, loc, value, index,
                                       ASR::down_cast<ASR::Tuple_t>(type)->m_type[i], nullptr);
                return false;
            } else {
                index = ASRUtils::EXPR(tmp);
            }
            ai.m_right = index;
            if (ASRUtils::is_character(*type)) {
                index = index_add_one(loc, index);
                ai.m_right = index;
                tmp = ASR::make_StringItem_t(al, loc, value, index, type, nullptr);
                return false;
            }
        }
        args.push_back(al, ai);
        return true;
    }

    void visit_Subscript(const AST::Subscript_t &x) {
        this->visit_expr(*x.m_value);
        if (using_args_attr) {
            if (AST::is_a<AST::Attribute_t>(*x.m_value)){
                AST::Attribute_t *attr = AST::down_cast<AST::Attribute_t>(x.m_value);
                if (AST::is_a<AST::Name_t>(*attr->m_value)) {
                    AST::Name_t *var_name = AST::down_cast<AST::Name_t>(attr->m_value);
                    std::string var = var_name->m_id;
                    ASR::symbol_t *st = current_scope->resolve_symbol(var);
                    ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(
                                    ASR::make_Var_t(al, x.base.base.loc, st));
                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 0);
                    this->visit_expr(*x.m_slice);
                    ASR::expr_t *index = ASRUtils::EXPR(tmp);
                    args.push_back(al, index);
                    tmp = attr_handler.eval_symbolic_get_argument(se, al, x.base.base.loc, args, diag);
                    using_args_attr = false;
                    return;
                }
            }
        }
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::ttype_t *type = ASRUtils::expr_type(value);
        Vec<ASR::array_index_t> args;
        args.reserve(al, 1);
        bool is_item = true;

        if (ASR::is_a<ASR::Set_t>(*type)) {
            throw SemanticError("'set' object is not subscriptable", x.base.base.loc);
        }

        if( !visit_SubscriptIndices(x.m_slice, args, value, type,
                                    is_item, x.base.base.loc) ) {
            return ;
        }

        if( is_item ) {
            for( size_t i = 0; i < args.size(); i++ ) {
                args.p[i].m_left = nullptr;
                args.p[i].m_step = nullptr;
            }
        }
        ASR::expr_t* v_Var = value;
        if( is_item ) {
            Vec<ASR::dimension_t> empty_dims;
            empty_dims.reserve(al, 1);
            if( ASR::is_a<ASR::CPtr_t>(*ASRUtils::type_get_past_pointer(type)) ) {
                throw SemanticError("Indexing CPtr typed expressions is not supported yet",
                                    x.base.base.loc);
            }
            type = ASRUtils::duplicate_type(al, ASRUtils::type_get_past_allocatable(
                ASRUtils::type_get_past_pointer(type)), &empty_dims);
            tmp = ASR::make_ArrayItem_t(al, x.base.base.loc, v_Var, args.p,
                        args.size(), type, ASR::arraystorageType::RowMajor, nullptr);
        } else {
            tmp = ASR::make_ArraySection_t(al, x.base.base.loc, v_Var, args.p,
                        args.size(), type, nullptr);
        }
    }

};


class SymbolTableVisitor : public CommonVisitor<SymbolTableVisitor> {
public:
    SymbolTable *global_scope;
    std::map<std::string, std::vector<std::string>> generic_procedures;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> generic_class_procedures;
    std::map<std::string, std::vector<std::string>> defined_op_procs;
    std::map<std::string, std::map<std::string, std::string>> class_procedures;
    std::vector<std::string> assgn_proc_names;
    std::string dt_name;
    bool in_module = false;
    bool in_submodule = false;
    bool is_interface = false;
    std::string interface_name = "";
    bool is_derived_type = false;
    std::vector<std::string> current_procedure_args;
    ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
    std::map<SymbolTable*, ASR::accessType> assgn;
    ASR::symbol_t *current_module_sym;
    std::vector<std::string> excluded_from_symtab;
    std::map<std::string, Vec<ASR::symbol_t* >> overload_defs;


    SymbolTableVisitor(Allocator &al, LocationManager &lm, SymbolTable *symbol_table,
        diag::Diagnostics &diagnostics, bool main_module, std::string module_name,
        std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir,
        std::vector<std::string> import_paths, bool allow_implicit_casting_)
      : CommonVisitor(al, lm, symbol_table, diagnostics, main_module, module_name, ast_overload,
            parent_dir, import_paths, allow_implicit_casting_), is_derived_type{false} {}


    ASR::symbol_t* resolve_symbol(const Location &loc, const std::string &sub_name) {
        SymbolTable *scope = current_scope;
        ASR::symbol_t *sub = scope->resolve_symbol(sub_name);
        if (!sub) {
            throw SemanticError("Symbol '" + sub_name + "' not declared", loc);
        }
        return sub;
    }

    void visit_Module(const AST::Module_t &x) {
        ASR::asr_t *tmp0 = nullptr;
        if (current_scope) {
            LCOMPILERS_ASSERT(current_scope->asr_owner);
            tmp0 = current_scope->asr_owner;
        } else {
            current_scope = al.make_new<SymbolTable>(nullptr);
            // Create the TU early, so that asr_owner is set, so that
            // ASRUtils::get_tu_symtab() can be used, which has an assert
            // for asr_owner.
            tmp0 = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);
        }
        LCOMPILERS_ASSERT(ASR::is_a<ASR::unit_t>(*tmp0) &&
            ASR::is_a<ASR::TranslationUnit_t>(*ASR::down_cast<ASR::unit_t>(tmp0)));
        global_scope = current_scope;

        ASR::Module_t* module_sym = nullptr;
        // Every module goes into a Module_t
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);

        ASR::asr_t *tmp1 = ASR::make_Module_t(al, x.base.base.loc,
                                    /* a_symtab */ current_scope,
                                    /* a_name */ s2c(al, module_name),
                                    nullptr,
                                    0,
                                    false, false);

        if (parent_scope->get_scope().find(module_name) != parent_scope->get_scope().end()) {
            throw SemanticError("Module '" + module_name + "' already defined", tmp1->loc);
        }
        module_sym = ASR::down_cast<ASR::Module_t>(ASR::down_cast<ASR::symbol_t>(tmp1));
        parent_scope->add_symbol(module_name, ASR::down_cast<ASR::symbol_t>(tmp1));
        current_module_dependencies.reserve(al, 1);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }

        LCOMPILERS_ASSERT(module_sym != nullptr);
        module_sym->m_dependencies = current_module_dependencies.p;
        module_sym->n_dependencies = current_module_dependencies.size();
        if (!overload_defs.empty()) {
            create_GenericProcedure(x.base.base.loc);
        }
        global_scope = nullptr;
        tmp = tmp0;
    }

    ASR::symbol_t* create_implicit_interface_function(Location &loc, ASR::FunctionType_t *func, std::string func_name) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);

        Vec<ASR::expr_t*> args;
        args.reserve(al, func->n_arg_types);
        std::string sym_name = to_lower(func_name);
        for (size_t i=0; i<func->n_arg_types; i++) {
            std::string arg_name = sym_name + "_arg_" + std::to_string(i);
            arg_name = to_lower(arg_name);
            ASR::symbol_t *v;
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec,
                    func->m_arg_types[i]);
            v = ASR::down_cast<ASR::symbol_t>(
                ASR::make_Variable_t(al, loc,
                current_scope, s2c(al, arg_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(), ASRUtils::intent_unspecified,
                nullptr, nullptr, ASR::storage_typeType::Default, func->m_arg_types[i],
                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required,
                false));
            current_scope->add_symbol(arg_name, v);
            LCOMPILERS_ASSERT(v != nullptr)
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, loc,
                v)));
        }

        ASR::expr_t *to_return = nullptr;
        if (func->m_return_var_type) {
            std::string return_var_name = sym_name + "_return_var_name";
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec,
                    func->m_return_var_type);
            ASR::asr_t *return_var = ASR::make_Variable_t(al, loc,
                current_scope, s2c(al, return_var_name), variable_dependencies_vec.p,
                variable_dependencies_vec.size(), ASRUtils::intent_return_var,
                nullptr, nullptr, ASR::storage_typeType::Default, func->m_return_var_type,
                nullptr, ASR::abiType::Source, ASR::Public, ASR::presenceType::Required,
                false);
            current_scope->add_symbol(return_var_name, ASR::down_cast<ASR::symbol_t>(return_var));
            to_return = ASRUtils::EXPR(ASR::make_Var_t(al, loc,
                ASR::down_cast<ASR::symbol_t>(return_var)));
        }

        tmp = ASRUtils::make_Function_t_util(
            al, loc,
            /* a_symtab */ current_scope,
            /* a_name */ s2c(al, sym_name),
            nullptr, 0,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_return_var */ to_return,
            ASR::abiType::BindC, ASR::accessType::Public, ASR::deftypeType::Interface,
            nullptr, false, false, false, false, false, nullptr, 0, false, false, false);
        current_scope = parent_scope;
        return ASR::down_cast<ASR::symbol_t>(tmp);
    }

    char* extract_keyword_val_from_decorator(AST::Call_t* x, std::string keyword) {
        for (size_t i=0; i < x->n_keywords; i++) {
            if (std::string(x->m_keywords[i].m_arg) == keyword) {
                if (AST::is_a<AST::ConstantStr_t>(*x->m_keywords[i].m_value)) {
                    std::string module_name = AST::down_cast<AST::ConstantStr_t>(
                                x->m_keywords[i].m_value)->m_value;
                    return s2c(al, module_name);
                } else {
                    throw SemanticError("module should be constant string in ccall",
                        x->base.base.loc);
                }
            }
        }
        return nullptr;
    }

    // Implement visit_While for Symbol Table visitor.
    void visit_While(const AST::While_t &/*x*/) {}

    // Implement visit_Delete for Symbol Table visitor.
    void visit_Delete(const AST::Delete_t &/*x*/) {}

    // Implement visit_Pass for Symbol Table visitor.
    void visit_Pass(const AST::Pass_t &/*x*/) {}

    // Implement visit_Return for Symbol Table visitor.
    void visit_Return(const AST::Return_t &/*x*/) {}

    // Implement visit_Raise for Symbol Table visitor.
    void visit_Raise(const AST::Raise_t &/*x*/) {}

    // Implement visit_Global for Symbol Table visitor.
    void visit_Global(const AST::Global_t &/*x*/) {}

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        dependencies.clear(al);
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.m_args.n_args);
        current_procedure_abi_type = ASR::abiType::Source;
        bool current_procedure_interface = false;
        bool overload = false;
        bool vectorize = false, is_inline = false, is_static = false;
        bool is_restriction = false;
        bool is_deterministic = false;
        bool is_side_effect_free = false;
        char *bindc_name=nullptr, *module_file=nullptr;
        if (x.n_decorator_list > 0) {
            for(size_t i=0; i<x.n_decorator_list; i++) {
                AST::expr_t *dec = x.m_decorator_list[i];
                if (AST::is_a<AST::Name_t>(*dec)) {
                    std::string name = AST::down_cast<AST::Name_t>(dec)->m_id;
                    if (name == "ccall") {
                        current_procedure_abi_type = ASR::abiType::BindC;
                        current_procedure_interface = true;
                    } else if (name == "ccallback" || name == "ccallable") {
                        current_procedure_abi_type = ASR::abiType::BindC;
                    } else if (name == "pythoncall" || name == "pythoncallable") {
                        current_procedure_abi_type = ASR::abiType::BindPython;
                        current_procedure_interface = (name == "pythoncall");
                    } else if (name == "jscall") {
                        current_procedure_abi_type = ASR::abiType::BindJS;
                    } else if (name == "overload") {
                        overload = true;
                    } else if (name == "interface") {
                        // TODO: Implement @interface
                    } else if (name == "vectorize") {
                        vectorize = true;
                    } else if (name == "restriction") {
                        is_restriction = true;
                    } else if (name == "inline") {
                        is_inline = true;
                    } else if (name == "static") {
                        is_static = true;
                    } else if (name == "lpython") {
                        throw SemanticError("`@lpython` decorator must be "
                            "run from CPython, not compiled using LPython",
                            dec->base.loc);
                    } else {
                        throw SemanticError("Decorator: " + name + " is not supported",
                            dec->base.loc);
                    }
                } else if (AST::is_a<AST::Call_t>(*dec)) {
                    AST::Call_t *call_d = AST::down_cast<AST::Call_t>(dec);
                    if (AST::is_a<AST::Name_t>(*call_d->m_func)) {
                        std::string name = AST::down_cast<AST::Name_t>(call_d->m_func)->m_id;
                        if (name == "ccall" || name == "ccallable") {
                            current_procedure_abi_type = ASR::abiType::BindC;
                            current_procedure_interface = (name == "ccall");
                            module_file = extract_keyword_val_from_decorator(call_d, "header");
                        } else if (name == "pythoncall") {
                            current_procedure_abi_type = ASR::abiType::BindPython;
                            current_procedure_interface = true;
                            module_file = extract_keyword_val_from_decorator(call_d, "module");
                        } else {
                            throw SemanticError("Unsupported Decorator type",
                                    x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("Only Name is supported in Call decorators for now",
                                x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Unsupported Decorator type",
                        x.base.base.loc);
                }
            }
        }

        std::string sym_name = x.m_name;
        if (overload) {
            std::string overload_number;
            if (overload_defs.find(sym_name) == overload_defs.end()){
                overload_number = "0";
                Vec<ASR::symbol_t *> v;
                v.reserve(al, 1);
                overload_defs[sym_name] = v;
            } else {
                overload_number = std::to_string(overload_defs[sym_name].size());
            }
            sym_name = "__lpython_overloaded_" + overload_number + "__" + sym_name;
        }
        if (parent_scope->get_scope().find(sym_name) != parent_scope->get_scope().end()) {
            throw SemanticError("Function " + std::string(x.m_name) +  " is already defined", x.base.base.loc);
        }
        bool is_allocatable = false;
        for (size_t i=0; i<x.m_args.n_args; i++) {
            char *arg=x.m_args.m_args[i].m_arg;
            Location loc = x.m_args.m_args[i].loc;
            if (x.m_args.m_args[i].m_annotation == nullptr) {
                throw SemanticError("Argument does not have a type", loc);
            }
            ASR::intentType s_intent = ASRUtils::intent_unspecified;
            AST::expr_t* arg_annotation_type = get_var_intent_and_annotation(x.m_args.m_args[i].m_annotation, s_intent);
            is_allocatable = false;
            ASR::ttype_t *arg_type = ast_expr_to_asr_type(x.base.base.loc, *arg_annotation_type,
                is_allocatable, true, current_procedure_abi_type, s_intent != ASR::intentType::Local);
            if ((s_intent == ASRUtils::intent_inout || s_intent == ASRUtils::intent_out)
                && !ASRUtils::is_aggregate_type(arg_type)) {
                throw SemanticError("Simple Type " + ASRUtils::type_to_str_python(arg_type)
                    + " cannot be intent InOut/Out", loc);
            }

            std::string arg_s = arg;
            ASR::expr_t *value = nullptr;
            ASR::expr_t *init_expr = nullptr;
            if (s_intent == ASRUtils::intent_unspecified) {
                s_intent = ASRUtils::intent_in;
                if (ASRUtils::is_array(arg_type)) {
                    s_intent = ASRUtils::intent_inout;
                }
            }
            ASR::storage_typeType storage_type =
                    ASR::storage_typeType::Default;
            if( ASR::is_a<ASR::Const_t>(*arg_type) ) {
                storage_type = ASR::storage_typeType::Parameter;
            }
            if (is_allocatable) {
                LCOMPILERS_ASSERT(storage_type != ASR::storage_typeType::Parameter);
                arg_type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, loc,
                   ASRUtils::type_get_past_pointer(arg_type)));
            }
            ASR::accessType s_access = ASR::accessType::Public;
            ASR::presenceType s_presence = ASR::presenceType::Required;
            bool value_attr = false;
            if (current_procedure_abi_type == ASR::abiType::BindC) {
                value_attr = true;
            }
            ASR::symbol_t *v;
            if (ASR::is_a<ASR::FunctionType_t>(*arg_type)) {
                ASR::FunctionType_t *func = ASR::down_cast<ASR::FunctionType_t>(arg_type);
                v = create_implicit_interface_function(loc, func, arg_s);
            } else {
                SetChar variable_dependencies_vec;
                variable_dependencies_vec.reserve(al, 1);
                ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, arg_type, init_expr, value);
                ASR::asr_t *_tmp = ASR::make_Variable_t(al, loc, current_scope,
                        s2c(al, arg_s), variable_dependencies_vec.p,
                        variable_dependencies_vec.size(),
                        s_intent, init_expr, value, storage_type, arg_type,
                        nullptr, current_procedure_abi_type, s_access, s_presence,
                        value_attr);
                v = ASR::down_cast<ASR::symbol_t>(_tmp);

            }
            if (current_scope->get_scope().find(arg_s) !=
                    current_scope->get_scope().end()) {
                ASR::symbol_t *orig_decl = current_scope->get_symbol(arg_s);
                throw SemanticError(diag::Diagnostic(
                    "Parameter is already declared",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("original declaration", {orig_decl->base.loc}, false),
                        diag::Label("redeclaration", {v->base.loc}),
                    }));
            }
            current_scope->add_symbol(arg_s, v);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                v)));
        }
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        if (current_procedure_interface) {
            deftype = ASR::deftypeType::Interface;
        }
        if (x.m_returns && !AST::is_a<AST::ConstantNone_t>(*x.m_returns)) {
            if (AST::is_a<AST::Name_t>(*x.m_returns) || AST::is_a<AST::Subscript_t>(*x.m_returns)) {
                std::string return_var_name = "_lpython_return_variable";
                is_allocatable = false;
                ASR::ttype_t *type = ast_expr_to_asr_type(x.m_returns->base.loc,
                    *x.m_returns, is_allocatable, true, current_procedure_abi_type, true);
                ASR::storage_typeType storage_type = ASR::storage_typeType::Default;
                if (is_allocatable) {
                    type = ASRUtils::TYPE(ASR::make_Allocatable_t(al, x.m_returns->base.loc,
                        ASRUtils::type_get_past_pointer(type)));
                }
                ASR::ttype_t* return_type_ = type;
                if( ASR::is_a<ASR::Const_t>(*type) ) {
                    return_type_ = ASR::down_cast<ASR::Const_t>(type)->m_type;
                }
                SetChar variable_dependencies_vec;
                variable_dependencies_vec.reserve(al, 1);
                ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type);
                ASR::asr_t *return_var = ASR::make_Variable_t(al, x.m_returns->base.loc,
                    current_scope, s2c(al, return_var_name), variable_dependencies_vec.p,
                    variable_dependencies_vec.size(), ASRUtils::intent_return_var,
                    nullptr, nullptr, storage_type, type, nullptr, current_procedure_abi_type, ASR::Public,
                    ASR::presenceType::Required, false);
                LCOMPILERS_ASSERT(current_scope->get_scope().find(return_var_name) == current_scope->get_scope().end())
                current_scope->add_symbol(return_var_name,
                        ASR::down_cast<ASR::symbol_t>(return_var));
                ASR::Variable_t* return_variable = ASR::down_cast<ASR::Variable_t>(ASR::down_cast<ASR::symbol_t>(return_var));
                ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
                    current_scope->get_symbol(return_var_name));
                tmp = ASRUtils::make_Function_t_util(
                    al, x.base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ s2c(al, sym_name),
                    dependencies.p, dependencies.size(),
                    /* a_args */ args.p,
                    /* n_args */ args.size(),
                    /* a_body */ nullptr,
                    /* n_body */ 0,
                    /* a_return_var */ ASRUtils::EXPR(return_var_ref),
                    current_procedure_abi_type,
                    s_access, deftype, bindc_name, vectorize, false, false, is_inline, is_static,
                    nullptr, 0,
                    is_restriction, is_deterministic, is_side_effect_free,
                    module_file);
                    return_variable->m_type = return_type_;
            } else {
                throw SemanticError("Return variable must be an identifier (Name AST node) or an array (Subscript AST node)",
                    x.m_returns->base.loc);
            }
        } else {
            bool is_pure = false, is_module = false;

            // This checks for internal function defintions as well.
            for (size_t i = 0; i < x.n_body; i++) {
                visit_stmt(*x.m_body[i]);
            }

            tmp = ASRUtils::make_Function_t_util(
                al, x.base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ s2c(al, sym_name),
                dependencies.p, dependencies.size(),
                /* a_args */ args.p,
                /* n_args */ args.size(),
                /* a_body */ nullptr,
                /* n_body */ 0,
                nullptr,
                current_procedure_abi_type,
                s_access, deftype, bindc_name,
                false, is_pure, is_module, is_inline, is_static,
                nullptr, 0,
                is_restriction, is_deterministic, is_side_effect_free,
                module_file);
        }
        ASR::symbol_t * t = ASR::down_cast<ASR::symbol_t>(tmp);
        parent_scope->add_symbol(sym_name, t);
        current_scope = parent_scope;
        if (overload) {
            overload_defs[x.m_name].push_back(al, t);
            ast_overload[(int64_t)&x] = t;
        }
    }

    void create_GenericProcedure(const Location &loc) {
        for(auto &p: overload_defs) {
            std::string def_name = p.first;
            tmp = ASR::make_GenericProcedure_t(al, loc, current_scope, s2c(al, def_name),
                        p.second.p, p.second.size(), ASR::accessType::Public);
            ASR::symbol_t *t = ASR::down_cast<ASR::symbol_t>(tmp);
            current_scope->add_symbol(def_name, t);
        }
    }

    void set_module_symbol(std::string &mod_sym, std::vector<std::string> &paths) {
        /*
            Extracts the directory and module name.
            If the import is of type from x.y.z import a
            then directory would be x/y/ and module name
            would be z.
            If the import is of type from x import a then
            directory would be empty and module name
            would be z.
        */
        size_t last_dot = mod_sym.find_last_of(".");
        std::string directory = "";
        if( last_dot != std::string::npos ) {
            directory = mod_sym.substr(0, last_dot);
            directory = replace(directory, "[.]", "/") + "/";
            mod_sym = mod_sym.substr(last_dot + 1, std::string::npos);
        }

        /*
            Return in case of lpython module.
            TODO: Restrict the check only if
            lpython is the same module as src/runtime/lpython
            For now its wild card i.e., lpython.py present
            in any directory other than src/runtime will also
            be ignored.
        */
        if (mod_sym == "lpython" || mod_sym == "numpy") {
            return ;
        }

        /*
            Search all the paths in order and stop
            when the desired module is found.
        */
        std::string path_found = "";
        for( auto& path: paths ) {
            if(is_directory(path + "/" + directory + mod_sym)) {
                // Directory i.e., x/y/__init__.py
                path_found = path + '/' + directory + mod_sym;
                mod_sym = "__init__";
                break;
            } else if(path_exists(path + "/" + directory + mod_sym + ".py")) {
                // File i.e., x/y.py
                path_found = path + '/' + directory;
                break;
            }
        }

        /*
            If module is not found in any of the paths
            specified and if its a directory
            then prioritise the directory itself.
        */
        if( path_found.empty() ) {
            if (is_directory(directory + mod_sym)) {
                // Directory i.e., x/__init__.py
                mod_sym = "__init__";
                path_found = directory + mod_sym;
            } else if (path_exists(directory + mod_sym + ".py")) {
                path_found = directory;
            }
        }

        // Update paths to contain only the found path (if found)
        // so that later load_module() should only use this path
        // to read the package/module file
        // load_module() need not search through all paths again
        if (path_found.empty()) {
            // include the runtime library dir so that later
            // runtime library modules could be imported
            paths = {get_runtime_library_dir()};
        } else {
            paths = {path_found};
        }
    }

    void visit_ImportFrom(const AST::ImportFrom_t &x) {
        if (!x.m_module) {
            throw SemanticError("Not implemented: The import statement must currently specify the module name", x.base.base.loc);
        }
        std::string msym = x.m_module; // Module name

        // Get the module, for now assuming it is not loaded, so we load it:
        ASR::symbol_t *t = nullptr; // current_scope->parent->resolve_symbol(msym);
        if (!t) {
            std::string rl_path = get_runtime_library_dir();
            std::vector<std::string> paths;
            for (auto &path:import_paths) {
                paths.push_back(path);
            }
            paths.push_back(rl_path);
            paths.push_back(parent_dir);

            bool lpython, enum_py, copy, sympy;
            set_module_symbol(msym, paths);
            t = (ASR::symbol_t*)(load_module(al, global_scope,
                msym, x.base.base.loc, diag, lm, false, paths, lpython, enum_py, copy, sympy,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); },
                allow_implicit_casting));
            if (lpython || enum_py || copy || sympy) {
                // TODO: For now we skip lpython import completely. Later on we should note what symbols
                // got imported from it, and give an error message if an annotation is used without
                // importing it.
                tmp = nullptr;
                return;
            }
            if (!t) {
                throw SemanticError("The module '" + msym + "' cannot be loaded",
                        x.base.base.loc);
            }
            if( msym == "__init__" ) {
                for( auto item: ASRUtils::symbol_symtab(t)->get_scope() ) {
                    if( ASR::is_a<ASR::ExternalSymbol_t>(*item.second) ) {
                        current_module_dependencies.push_back(al,
                            ASR::down_cast<ASR::ExternalSymbol_t>(item.second)->m_module_name);
                    }
                }
            } else {
                current_module_dependencies.push_back(al, s2c(al, msym));
            }
        }

        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
        int i = -1;
        for (size_t j=0; j<x.n_names; j++) {
            std::string remote_sym = x.m_names[j].m_name;
            i++;
            if( procedures_db.is_function_to_be_ignored(msym, remote_sym) ) {
                continue ;
            }
            std::string new_sym_name = ASRUtils::get_mangled_name(m, remote_sym);
            if (x.m_names[j].m_asname) {
                new_sym_name = ASRUtils::get_mangled_name(m, x.m_names[j].m_asname);
            }
            ASR::symbol_t *t = import_from_module(al, m, current_scope, msym,
                                remote_sym, new_sym_name, x.m_names[i].loc, true);
            if (current_scope->get_scope().find(new_sym_name) != current_scope->get_scope().end()) {
                ASR::symbol_t *old_sym = current_scope->get_scope().find(new_sym_name)->second;
                diag.add(diag::Diagnostic(
                    "The symbol '" + new_sym_name + "' imported from " + std::string(m->m_name) +" will shadow the existing symbol '" + new_sym_name + "'",
                    diag::Level::Warning, diag::Stage::Semantic, {
                        diag::Label("old symbol", {old_sym->base.loc}),
                        diag::Label("new symbol", {t->base.loc}),
                    })
                );
                current_scope->overwrite_symbol(new_sym_name, t);
            } else {
                current_scope->add_symbol(new_sym_name, t);
            }
        }

        tmp = nullptr;
    }

    void visit_Import(const AST::Import_t &x) {
        ASR::symbol_t *t = nullptr;
        std::string rl_path = get_runtime_library_dir();
        std::vector<std::string> paths;
        for (auto &path:import_paths) {
            paths.push_back(path);
        }
        paths.push_back(rl_path);
        paths.push_back(parent_dir);
        std::vector<std::string> mods;
        for (size_t i=0; i<x.n_names; i++) {
            mods.push_back(x.m_names[i].m_name);
        }
        for (auto &mod_sym : mods) {
            bool lpython, enum_py, copy, sympy;
            set_module_symbol(mod_sym, paths);
            t = (ASR::symbol_t*)(load_module(al, global_scope,
                mod_sym, x.base.base.loc, diag, lm, false, paths, lpython, enum_py, copy, sympy,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); },
                allow_implicit_casting));
            if (lpython || enum_py || copy || sympy) {
                // TODO: For now we skip lpython import completely. Later on we should note what symbols
                // got imported from it, and give an error message if an annotation is used without
                // importing it.
                tmp = nullptr;
                continue;
            }
            if (!t) {
                throw SemanticError("The module '" + mod_sym + "' cannot be loaded",
                        x.base.base.loc);
            }
        }
    }

    void visit_AugAssign(const AST::AugAssign_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_AnnAssign(const AST::AnnAssign_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_Assign(const AST::Assign_t &x) {
        /**
         *  Type variables have to be put in the symbol table before checking function definitions.
         *  This describes a different treatment for Assign in the form of `T = TypeVar('T', ...)`
         */
        if (x.n_targets == 1 && AST::is_a<AST::Call_t>(*x.m_value)) {
            AST::Call_t *rh = AST::down_cast<AST::Call_t>(x.m_value);
            if (AST::is_a<AST::Name_t>(*rh->m_func)) {
                AST::Name_t *tv = AST::down_cast<AST::Name_t>(rh->m_func);
                std::string f_name = tv->m_id;
                if (strcmp(s2c(al, f_name), "TypeVar") == 0 &&
                        rh->n_args > 0 && AST::is_a<AST::ConstantStr_t>(*rh->m_args[0])) {
                    if (AST::is_a<AST::Name_t>(*x.m_targets[0])) {
                        std::string tvar_name = AST::down_cast<AST::Name_t>(x.m_targets[0])->m_id;
                        // Check if the type variable name is a reserved type keyword
                        const char* type_list[15]
                            = { "list", "set", "dict", "tuple",
                                "i8", "i16", "i32", "i64", "f32",
                                "f64", "c32", "c64", "str", "bool", "i1"};
                        for (int i = 0; i < 15; i++) {
                            if (strcmp(s2c(al, tvar_name), type_list[i]) == 0) {
                                throw SemanticError(tvar_name + " is a reserved type, consider a different type variable name",
                                    x.base.base.loc);
                            }
                        }
                        // Check if the type variable is already defined
                        if (current_scope->get_scope().find(tvar_name) !=
                                current_scope->get_scope().end()) {
                            ASR::symbol_t *orig_decl = current_scope->get_symbol(tvar_name);
                            throw SemanticError(diag::Diagnostic(
                                "Variable " + tvar_name + " is already declared in the same scope",
                                diag::Level::Error, diag::Stage::Semantic, {
                                    diag::Label("original declaration", {orig_decl->base.loc}, false),
                                    diag::Label("redeclaration", {x.base.base.loc}),
                            }));
                        }

                        // Build ttype
                        Vec<ASR::dimension_t> dims;
                        dims.reserve(al, 4);

                        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_TypeParameter_t(al, x.base.base.loc,
                            s2c(al, tvar_name)));
                        type = ASRUtils::make_Array_t_util(al, x.base.base.loc, type, dims.p, dims.size());

                        ASR::expr_t *value = nullptr;
                        ASR::expr_t *init_expr = nullptr;
                        ASR::intentType s_intent = ASRUtils::intent_local;
                        ASR::storage_typeType storage_type = ASR::storage_typeType::Default;
                        if( ASR::is_a<ASR::Const_t>(*type) ) {
                            storage_type = ASR::storage_typeType::Parameter;
                        }
                        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
                        ASR::accessType s_access = ASR::accessType::Public;
                        ASR::presenceType s_presence = ASR::presenceType::Required;
                        bool value_attr = false;

                        SetChar variable_dependencies_vec;
                        variable_dependencies_vec.reserve(al, 1);
                        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, type, init_expr, value);
                        // Build the variable and add it to the scope
                        ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                            s2c(al, tvar_name), variable_dependencies_vec.p, variable_dependencies_vec.size(),
                            s_intent, init_expr, value, storage_type, type, nullptr, current_procedure_abi_type,
                            s_access, s_presence, value_attr);
                        current_scope->add_symbol(tvar_name, ASR::down_cast<ASR::symbol_t>(v));

                        tmp = nullptr;

                        return;
                    } else {
                        // This error might need to be further elaborated
                        throw SemanticError("Type variable must be a variable", x.base.base.loc);
                    }
                }
            }
        }
    }

    void visit_Expr(const AST::Expr_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_For(const AST::For_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_Assert(const AST::Assert_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_If(const AST::If_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }

    void visit_Call(const AST::Call_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
};

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, LocationManager &lm,
        SymbolTable* symtab, const AST::Module_t &ast,
        diag::Diagnostics &diagnostics, bool main_module, std::string module_name,
        std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir,
        std::vector<std::string> import_paths, bool allow_implicit_casting)
{
    SymbolTableVisitor v(al, lm, symtab, diagnostics, main_module, module_name, ast_overload,
        parent_dir, import_paths, allow_implicit_casting);
    try {
        v.visit_Module(ast);
    } catch (const SemanticError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const SemanticAbort &) {
        Error error;
        return error;
    }
    ASR::asr_t *unit = v.tmp;
    return unit;
}

class BodyVisitor : public CommonVisitor<BodyVisitor> {
private:

public:
    ASR::asr_t *asr;
    std::vector<ASR::symbol_t*> do_loop_variables;
    bool using_func_attr = false;

    BodyVisitor(Allocator &al, LocationManager &lm, ASR::asr_t *unit, diag::Diagnostics &diagnostics,
         bool main_module, std::string module_name, std::map<int, ASR::symbol_t*> &ast_overload,
         bool allow_implicit_casting_)
         : CommonVisitor(al, lm, nullptr, diagnostics, main_module, module_name, ast_overload, "", {}, allow_implicit_casting_),
         asr{unit}
         {}

    // Transforms statements to a list of ASR statements
    // In addition, it also inserts the following nodes if needed:
    //   * ImplicitDeallocate
    // The `body` Vec must already be reserved
    void transform_stmts(Vec<ASR::stmt_t*> &body, size_t n_body, AST::stmt_t **m_body) {
        tmp = nullptr;
        tmp_vec.clear();
        Vec<ASR::stmt_t*>* current_body_copy = current_body;
        current_body = &body;
        for (size_t i=0; i<n_body; i++) {
            // Visit the statement
            this->visit_stmt(*m_body[i]);
            if (tmp != nullptr) {
                if (ASR::is_a<ASR::expr_t>(*tmp)) {
                    tmp = make_dummy_assignment(ASRUtils::EXPR(tmp));
                }
                ASR::stmt_t* tmp_stmt = ASRUtils::STMT(tmp);
                body.push_back(al, tmp_stmt);
            } else if (!tmp_vec.empty()) {
                for (auto t: tmp_vec) {
                    if (t != nullptr) {
                        if (ASR::is_a<ASR::expr_t>(*t)) {
                            t = make_dummy_assignment(ASRUtils::EXPR(t));
                        }
                        ASR::stmt_t* tmp_stmt = ASRUtils::STMT(t);
                        body.push_back(al, tmp_stmt);
                    }
                }
                tmp_vec.clear();
            }
            // To avoid last statement to be entered twice once we exit this node
            tmp = nullptr;
            tmp_vec.clear();
        }
        current_body = current_body_copy;
    }

    void visit_Module(const AST::Module_t &x) {
        ASR::TranslationUnit_t *unit = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
        current_scope = unit->m_symtab;
        LCOMPILERS_ASSERT(current_scope != nullptr);
        ASR::symbol_t* module_sym = nullptr;
        ASR::Module_t* mod = nullptr;

        LCOMPILERS_ASSERT(module_name.size() > 0);
        module_sym = current_scope->get_symbol(module_name);
        mod = ASR::down_cast<ASR::Module_t>(module_sym);
        LCOMPILERS_ASSERT(mod != nullptr);
        current_scope = mod->m_symtab;
        LCOMPILERS_ASSERT(current_scope != nullptr);

        Vec<ASR::asr_t*> items;
        items.reserve(al, 4);
        current_module_dependencies.reserve(al, 1);
        for (size_t i=0; i<x.n_body; i++) {
            tmp = nullptr;
            tmp_vec.clear();
            visit_stmt(*x.m_body[i]);
            if (tmp) {
                items.push_back(al, tmp);
            } else if (!tmp_vec.empty()) {
                for (auto t: tmp_vec) {
                    if (t) items.push_back(al, t);
                }
                // Ensure that statements in tmp_vec are used only once.
                tmp_vec.clear();
            }
        }

        for( size_t i = 0; i < mod->n_dependencies; i++ ) {
            current_module_dependencies.push_back(al, mod->m_dependencies[i]);
        }
        mod->m_dependencies = current_module_dependencies.p;
        mod->n_dependencies = current_module_dependencies.n;

        if (global_init.n > 0) {
            // unit->m_items is used and set to nullptr in the
            // `pass_wrap_global_stmts_into_function` pass
            unit->m_items = global_init.p;
            unit->n_items = global_init.size();
            std::string func_name = module_name + "global_init";
            LCompilers::PassOptions pass_options;
            pass_options.run_fun = func_name;
            pass_wrap_global_stmts(al, *unit, pass_options);

            ASR::symbol_t *f_sym = unit->m_symtab->get_symbol(func_name);
            if (f_sym) {
                // Add the `global_initilaizer` function into the
                // module and later call this function to initialize the
                // global variables like list, ...
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(f_sym);
                f->m_symtab->parent = mod->m_symtab;
                mod->m_symtab->add_symbol(func_name, (ASR::symbol_t *) f);
                // Erase the function in TranslationUnit
                unit->m_symtab->erase_symbol(func_name);
            }
            global_init.p = nullptr;
            global_init.n = 0;
        }

        if (items.n > 0) {
            unit->m_items = items.p;
            unit->n_items = items.size();
            std::string func_name = module_name + "global_stmts";
            // Wrap all the global statements into a Function
            LCompilers::PassOptions pass_options;
            pass_options.run_fun = func_name;
            pass_wrap_global_stmts(al, *unit, pass_options);

            ASR::symbol_t *f_sym = unit->m_symtab->get_symbol(func_name);
            if (f_sym) {
                // Add the `global_statements` function into the
                // module and later call this function to execute the
                // global_statements
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(f_sym);
                f->m_symtab->parent = mod->m_symtab;
                mod->m_symtab->add_symbol(func_name, (ASR::symbol_t *) f);
                // Erase the function in TranslationUnit
                unit->m_symtab->erase_symbol(func_name);
            }
            items.p = nullptr;
            items.n = 0;
        }

        tmp = asr;
    }

    void handle_fn(const AST::FunctionDef_t &x, ASR::Function_t &v) {
        current_scope = v.m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        Vec<ASR::symbol_t*> rts;
        rts.reserve(al, 4);
        dependencies.clear(al);
        transform_stmts(body, x.n_body, x.m_body);
        for (const auto &rt: rt_vec) { rts.push_back(al, rt); }
        v.m_body = body.p;
        v.n_body = body.size();
        ASR::FunctionType_t* v_func_type = ASR::down_cast<ASR::FunctionType_t>(v.m_function_signature);
        v_func_type->m_restrictions = rts.p;
        v_func_type->n_restrictions = rts.size();
        v.m_dependencies = dependencies.p;
        v.n_dependencies = dependencies.size();
        rt_vec.clear();
    }

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->get_symbol(x.m_name);

        if (t == nullptr) {
            // Throw Not implemented error.
            throw SemanticError("Internal FunctionDef: Not implemented", x.base.base.loc);
        }

        if (ASR::is_a<ASR::Function_t>(*t)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(t);
            if (!ASRUtils::get_FunctionType(f)->m_is_restriction) {
                handle_fn(x, *f);
            }
        } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
            ASR::symbol_t *s = ast_overload[(int64_t)&x];
            if (ASR::is_a<ASR::Function_t>(*s)) {
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
                handle_fn(x, *f);
            } else {
                LCOMPILERS_ASSERT(false);
            }
        } else {
            LCOMPILERS_ASSERT(false);
        }
        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Import(const AST::Import_t &x) {
        // All the modules are imported in the SymbolTable visitor
        for (size_t i = 0; i < x.n_names; i++) {
            std::string mod_name = x.m_names[i].m_name;
            ASR::symbol_t *mod_sym = current_scope->resolve_symbol(mod_name);
            if (mod_sym) {
                ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
                get_calls_to_global_init_and_stmts(al, x.base.base.loc, current_scope, mod, tmp_vec);
            }
        }
    }

    void visit_ImportFrom(const AST::ImportFrom_t &x) {
        // Handled by SymbolTableVisitor already
        std::string mod_name = x.m_module;
        for (size_t i = 0; i < x.n_names; i++) {
            if (x.m_names[i].m_asname) {
                imported_functions[x.m_names[i].m_asname] = mod_name;
            }
            else {
                imported_functions[x.m_names[i].m_name] = mod_name;
            }
        }
        ASR::symbol_t *mod_sym = current_scope->resolve_symbol(mod_name);
        if (mod_sym) {
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
            get_calls_to_global_init_and_stmts(al, x.base.base.loc, current_scope, mod, tmp_vec);
        }
        tmp = nullptr;
    }

    void visit_AnnAssign(const AST::AnnAssign_t &x) {
        // We treat this as a declaration
        std::string var_name;
        std::string var_annotation;
        if (AST::is_a<AST::Name_t>(*x.m_target)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(x.m_target);
            var_name = n->m_id;
        } else {
            throw SemanticError("Only Name supported for now as LHS of annotated assignment",
                x.base.base.loc);
        }

        if (current_scope->get_scope().find(var_name) !=
                current_scope->get_scope().end()) {
            ASR::symbol_t *orig_decl = current_scope->get_symbol(var_name);
            throw SemanticError(diag::Diagnostic(
                "Symbol is already declared in the same scope",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("original declaration", {orig_decl->base.loc}, false),
                    diag::Label("redeclaration", {x.base.base.loc}),
                }));
        }
        ASR::expr_t *init_expr = nullptr;
        visit_AnnAssignUtil(x, var_name, init_expr);
    }

    void visit_Delete(const AST::Delete_t &x) {
        if (x.n_targets == 0) {
            throw SemanticError("Delete statement must be operated on at least one target",
                    x.base.base.loc);
        }
        Vec<ASR::expr_t*> targets;
        targets.reserve(al, x.n_targets);
        for (size_t i=0; i<x.n_targets; i++) {
            AST::expr_t *target = x.m_targets[i];
            if (AST::is_a<AST::Name_t>(*target)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(target);
                std::string var_name = n->m_id;
                if (!current_scope->resolve_symbol(var_name)) {
                    throw SemanticError("Symbol is not declared",
                            x.base.base.loc);
                }
                ASR::symbol_t *s = current_scope->resolve_symbol(var_name);
                targets.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, target->base.loc, s)));
            } else {
                throw SemanticError("Only Name supported for now as target of Delete",
                        x.base.base.loc);
            }
        }
        tmp = ASR::make_ExplicitDeallocate_t(al, x.base.base.loc, targets.p,
                targets.size());
    }

    bool check_is_assign_to_input_param(AST::expr_t* x) {
        if (AST::is_a<AST::Name_t>(*x)) {
            AST::Name_t* n = AST::down_cast<AST::Name_t>(x);
            ASR::symbol_t* s = current_scope->resolve_symbol(n->m_id);
            if (!s) {
                throw SemanticError("Variable: '" + std::string(n->m_id) + "' is not declared",
                        x->base.loc);
            }
            ASR::Variable_t* v =  ASR::down_cast<ASR::Variable_t>(s);
            if (v->m_intent == ASR::intentType::In) {
                std::string msg = "Hint: create a new local variable with a different name";
                if (ASRUtils::is_aggregate_type(v->m_type)) {
                    msg = "Use InOut[" + ASRUtils::type_to_str_python(v->m_type) + "] to allow assignment";
                }
                diag.add(diag::Diagnostic(
                    "Assignment to an input function parameter `"
                    + std::string(v->m_name) + "` is not allowed",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label(msg, {x->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            return true;
        } else if (AST::is_a<AST::Subscript_t>(*x)) {
            AST::Subscript_t* s = AST::down_cast<AST::Subscript_t>(x);
            return check_is_assign_to_input_param(s->m_value);
        } else if (AST::is_a<AST::Attribute_t>(*x)) {
            AST::Attribute_t* s = AST::down_cast<AST::Attribute_t>(x);
            return check_is_assign_to_input_param(s->m_value);
        } else if (AST::is_a<AST::Tuple_t>(*x)) {
            AST::Tuple_t* t = AST::down_cast<AST::Tuple_t>(x);
            bool is_assign_to_input = false;
            for (size_t i = 0; i < t->n_elts && !is_assign_to_input; i++) {
                is_assign_to_input = is_assign_to_input || check_is_assign_to_input_param(t->m_elts[i]);
            }
            return is_assign_to_input;
        }else {
            throw SemanticError("Unsupported type in check_is_assign_to_input_param()", x->base.loc);
        }
    }

    bool visit_SubscriptUtil(const AST::Subscript_t &x, const AST::Assign_t &assign_node,
                             ASR::expr_t *tmp_value, int32_t recursion_level) {
        if (AST::is_a<AST::Name_t>(*x.m_value)) {
            std::string name = AST::down_cast<AST::Name_t>(x.m_value)->m_id;
            ASR::symbol_t *s = current_scope->resolve_symbol(name);
            if (!s) {
                throw SemanticError("Variable: '" + name + "' is not declared",
                        x.base.base.loc);
            }
            ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
            ASR::ttype_t *type = v->m_type;
            if (ASR::is_a<ASR::Dict_t>(*type)) {
                this->visit_expr(*x.m_slice);
                ASR::expr_t *key = ASRUtils::EXPR(tmp);
                ASR::expr_t* se = ASR::down_cast<ASR::expr_t>(
                        ASR::make_Var_t(al, x.base.base.loc, s));
                if( recursion_level == 0 ) {
                    // dict insert case;
                    ASR::ttype_t *key_type = ASR::down_cast<ASR::Dict_t>(type)->m_key_type;
                    ASR::ttype_t *value_type = ASR::down_cast<ASR::Dict_t>(type)->m_value_type;
                    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(key), key_type)) {
                        std::string ktype = ASRUtils::type_to_str_python(ASRUtils::expr_type(key));
                        std::string totype = ASRUtils::type_to_str_python(key_type);
                        diag.add(diag::Diagnostic(
                            "Type mismatch in dictionary key, the types must be compatible",
                            diag::Level::Error, diag::Stage::Semantic, {
                                diag::Label("type mismatch (found: '" + ktype + "', expected: '" + totype + "')",
                                        {key->base.loc})
                            })
                        );
                        throw SemanticAbort();
                    }
                    if (tmp_value == nullptr) {
                        if (AST::is_a<AST::List_t>(*assign_node.m_value)) {
                            LCOMPILERS_ASSERT(AST::down_cast<AST::List_t>(assign_node.m_value)->n_elts == 0);
                            Vec<ASR::expr_t*> list_ele;
                            list_ele.reserve(al, 1);
                            tmp_value = ASRUtils::EXPR(ASR::make_ListConstant_t(al, assign_node.base.base.loc,
                                                        list_ele.p, list_ele.size(), value_type));
                        } else if (AST::is_a<AST::Dict_t>(*assign_node.m_value)) {
                            LCOMPILERS_ASSERT(AST::down_cast<AST::Dict_t>(assign_node.m_value)->n_keys == 0);
                            Vec<ASR::expr_t*> dict_ele;
                            dict_ele.reserve(al, 1);
                            tmp_value = ASRUtils::EXPR(ASR::make_DictConstant_t(al, assign_node.base.base.loc,
                                            dict_ele.p, dict_ele.size(), dict_ele.p, dict_ele.size(), value_type));
                        }
                    }
                    if (!ASRUtils::check_equal_type(ASRUtils::expr_type(tmp_value), value_type)) {
                        std::string vtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(tmp_value));
                        std::string totype = ASRUtils::type_to_str_python(value_type);
                        diag.add(diag::Diagnostic(
                            "Type mismatch in dictionary value, the types must be compatible",
                            diag::Level::Error, diag::Stage::Semantic, {
                                diag::Label("type mismatch (found: '" + vtype + "', expected: '" + totype + "')",
                                        {tmp_value->base.loc})
                            })
                        );
                        throw SemanticAbort();
                    }
                    tmp = nullptr;
                    tmp_vec.push_back(make_DictInsert_t(al, x.base.base.loc, se, key, tmp_value));
                }
                else {
                    tmp = make_DictItem_t(al, x.base.base.loc, se, key, nullptr,
                                        ASR::down_cast<ASR::Dict_t>(type)->m_value_type, nullptr);
                }
                return true;
            } else if (ASRUtils::is_immutable(type)) {
                throw SemanticError("'" + ASRUtils::type_to_str_python(type) + "' object does not support"
                    " item assignment", x.base.base.loc);
            }
        } else if( AST::is_a<AST::Subscript_t>(*x.m_value) ) {
            AST::Subscript_t *sb = AST::down_cast<AST::Subscript_t>(x.m_value);
            bool return_val = visit_SubscriptUtil(*sb, assign_node, tmp_value, recursion_level + 1);
            if( return_val && tmp ) {
                ASR::expr_t *dict = ASRUtils::EXPR(tmp);
                this->visit_expr(*x.m_slice);
                ASR::expr_t *key = ASRUtils::EXPR(tmp);
                if( recursion_level == 0 ) {
                    tmp_vec.push_back(make_DictInsert_t(al, x.base.base.loc, dict, key, tmp_value));
                }
                else {
                    tmp = make_DictItem_t(al, x.base.base.loc, dict, key, nullptr,
                                        ASR::down_cast<ASR::Dict_t>(ASRUtils::expr_type(dict))->m_value_type, nullptr);
                }
            }
            return return_val;
        }
        return false;
    }

    void visit_Assign(const AST::Assign_t &x) {
        ASR::expr_t *target, *assign_value = nullptr, *tmp_value;
        ASR::expr_t* assign_asr_target_copy = assign_asr_target;
        this->visit_expr(*x.m_targets[0]);
        assign_asr_target = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        assign_asr_target = assign_asr_target_copy;
        if (tmp) {
            if (ASR::is_a<ASR::stmt_t>(*tmp)) {
                // This happens for c_p_pointer() and
                // empty() (if target is of type allocatable)
                return;
            }
            // This happens if `m.m_value` is `empty`, such as in:
            // a = empty(16)
            // We skip this statement for now, the array is declared
            // by the annotation.
            // TODO: enforce that empty(), ones(), zeros() is called
            // for every declaration.
            assign_value = ASRUtils::EXPR(tmp);
        }
        for (size_t i=0; i<x.n_targets; i++) {
            tmp_value = assign_value;
            check_is_assign_to_input_param(x.m_targets[i]);
            if (AST::is_a<AST::Subscript_t>(*x.m_targets[i])) {
                AST::Subscript_t *sb = AST::down_cast<AST::Subscript_t>(x.m_targets[i]);
                if( visit_SubscriptUtil(*sb, x, tmp_value, 0) ) {
                    continue;
                }
            } else if (AST::is_a<AST::Attribute_t>(*x.m_targets[i])) {
                AST::Attribute_t *attr = AST::down_cast<AST::Attribute_t>(x.m_targets[i]);
                if (AST::is_a<AST::Name_t>(*attr->m_value)) {
                    std::string name = AST::down_cast<AST::Name_t>(attr->m_value)->m_id;
                    ASR::symbol_t *s = current_scope->get_symbol(name);
                    if (!s) {
                        throw SemanticError("Variable: '" + name + "' is not declared",
                                x.base.base.loc);
                    }
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
                    ASR::ttype_t *type = v->m_type;
                    if (ASRUtils::is_immutable(type)) {
                        throw SemanticError("readonly attribute", x.base.base.loc);
                    }
                }
            }
            this->visit_expr(*x.m_targets[i]);
            target = ASRUtils::EXPR(tmp);
            ASR::ttype_t *target_type = ASRUtils::expr_type(target);
            if (tmp_value == nullptr) {
                if (AST::is_a<AST::List_t>(*x.m_value)) {
                    LCOMPILERS_ASSERT(AST::down_cast<AST::List_t>(x.m_value)->n_elts == 0);
                    Vec<ASR::expr_t*> list_ele;
                    list_ele.reserve(al, 1);
                    tmp_value = ASRUtils::EXPR(ASR::make_ListConstant_t(al, x.base.base.loc, list_ele.p,
                                    list_ele.size(), target_type));
                } else if (AST::is_a<AST::Dict_t>(*x.m_value)) {
                    LCOMPILERS_ASSERT(AST::down_cast<AST::Dict_t>(x.m_value)->n_keys == 0);
                    Vec<ASR::expr_t*> dict_ele;
                    dict_ele.reserve(al, 1);
                    tmp_value = ASRUtils::EXPR(ASR::make_DictConstant_t(al, x.base.base.loc, dict_ele.p,
                                    dict_ele.size(), dict_ele.p, dict_ele.size(), target_type));
                }
            }
            if (!tmp_value) continue;
            ASR::ttype_t *value_type = ASRUtils::expr_type(tmp_value);
            if( ASR::is_a<ASR::Pointer_t>(*target_type) &&
                ASR::is_a<ASR::Var_t>(*target) ) {
                if( !ASRUtils::check_equal_type(target_type, value_type) ) {
                    throw SemanticError("Casting not supported for different pointer types. Received "
                                        "target pointer type, " + ASRUtils::type_to_str_python(target_type) +
                                        " and value pointer type, " + ASRUtils::type_to_str_python(value_type),
                                        x.base.base.loc);
                }
                tmp = nullptr;
                tmp_vec.push_back(ASR::make_Assignment_t(al, x.base.base.loc, target,
                                    tmp_value, nullptr));
                continue;
            }
            if( ASR::is_a<ASR::Const_t>(*ASRUtils::expr_type(target)) ) {
                throw SemanticError("Targets with " +
                                    ASRUtils::type_to_str_python(ASRUtils::expr_type(target)) +
                                    " type cannot be re-assigned.",
                                    target->base.loc);
            }
            cast_helper(target, tmp_value, true);
            value_type = ASRUtils::expr_type(tmp_value);
            if (!ASRUtils::check_equal_type(target_type, value_type)) {
                std::string ltype = ASRUtils::type_to_str_python(target_type);
                std::string rtype = ASRUtils::type_to_str_python(value_type);
                diag.add(diag::Diagnostic(
                    "Type mismatch in assignment, the types must be compatible",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                                {target->base.loc, tmp_value->base.loc})
                    })
                );
                throw SemanticAbort();
            }
            ASR::stmt_t *overloaded=nullptr;
            tmp = nullptr;
            if (target->type == ASR::exprType::Var) {
                ASR::Var_t *var = ASR::down_cast<ASR::Var_t>(target);
                ASR::symbol_t *sym = var->m_v;
                if (do_loop_variables.size() > 0 && std::find(do_loop_variables.begin(), do_loop_variables.end(), sym) != do_loop_variables.end()) {
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(sym);
                    std::string var_name = std::string(v->m_name);
                    throw SemanticError("Assignment to loop variable `" + std::string(to_lower(var_name)) +"` is not allowed", target->base.loc);
                }
            }
            tmp_vec.push_back(ASR::make_Assignment_t(al, x.base.base.loc, target, tmp_value,
                                    overloaded));
        }
        // to make sure that we add only those statements in tmp_vec
        tmp = nullptr;
    }

    void visit_Assert(const AST::Assert_t &x) {
        this->visit_expr(*x.m_test);
        ASR::expr_t *test = ASRUtils::EXPR(tmp);
        ASR::expr_t *msg = nullptr;
        if (x.m_msg != nullptr) {
            this->visit_expr(*x.m_msg);
            msg = ASRUtils::EXPR(tmp);
        }
        tmp = ASR::make_Assert_t(al, x.base.base.loc, test, msg);
    }

    void visit_List(const AST::List_t &x) {
        Vec<ASR::expr_t*> list;
        list.reserve(al, x.n_elts + 1);
        ASR::ttype_t *type = nullptr;
        ASR::expr_t *expr = nullptr;
        if( x.n_elts > 0 ) {
            this->visit_expr(*x.m_elts[0]);
            expr = ASRUtils::EXPR(tmp);
            type = ASRUtils::expr_type(expr);
            list.push_back(al, expr);
            for (size_t i = 1; i < x.n_elts; i++) {
                this->visit_expr(*x.m_elts[i]);
                expr = ASRUtils::EXPR(tmp);
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(expr), type)) {
                    throw SemanticError("All List elements must be of the same type for now",
                        x.base.base.loc);
                }
                list.push_back(al, expr);
            }
        } else {
            if( assign_asr_target == nullptr ) {
                tmp = nullptr;
                return ;
            }
            type = ASRUtils::get_contained_type(
                ASRUtils::type_get_past_const(ASRUtils::expr_type(assign_asr_target)));
        }
        ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, x.base.base.loc, type));
        tmp = ASR::make_ListConstant_t(al, x.base.base.loc, list.p,
            list.size(), list_type);
    }

    ASR::expr_t* for_iterable_helper(std::string var_name, const Location& loc,
        std::string& explicit_iter_name_) {
        auto loop_src_var_symbol = current_scope->resolve_symbol(var_name);
        LCOMPILERS_ASSERT(loop_src_var_symbol!=nullptr);
        auto loop_src_var_ttype = ASRUtils::symbol_type(loop_src_var_symbol);
        ASR::ttype_t* int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
        // create a new variable called/named __explicit_iterator of type i32 and add it to symbol table
        std::string explicit_iter_name = current_scope->get_unique_name("__explicit_iterator", false);
        explicit_iter_name_ = explicit_iter_name;
        ASR::storage_typeType storage_type = ASR::storage_typeType::Default;
        if( ASR::is_a<ASR::Const_t>(*int_type) ) {
            storage_type = ASR::storage_typeType::Parameter;
        }
        SetChar variable_dependencies_vec;
        variable_dependencies_vec.reserve(al, 1);
        ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, int_type);
        auto explicit_iter_variable = ASR::make_Variable_t(al, loc,
            current_scope, s2c(al, explicit_iter_name),
            variable_dependencies_vec.p, variable_dependencies_vec.size(),
            ASR::intentType::Local, nullptr, nullptr, storage_type,
            int_type, nullptr, ASR::abiType::Source, ASR::accessType::Public,
            ASR::presenceType::Required, false
        );
        current_scope->add_symbol(explicit_iter_name,
                        ASR::down_cast<ASR::symbol_t>(explicit_iter_variable));
        // make loop_end = len(loop_src_var), where loop_src_var is the variable over which
        // we are iterating the for in loop
        LCOMPILERS_ASSERT(loop_src_var_symbol != nullptr);
        auto loop_src_var = ASR::make_Var_t(al, loc, loop_src_var_symbol);
        if (ASR::is_a<ASR::Character_t>(*loop_src_var_ttype)) {
            return ASRUtils::EXPR(ASR::make_StringLen_t(al, loc,
                                  ASRUtils::EXPR(loop_src_var),
                                  int_type, nullptr));
        } else if (ASR::is_a<ASR::List_t>(*loop_src_var_ttype)) {
            return ASRUtils::EXPR(ASR::make_ListLen_t(al, loc,
                                  ASRUtils::EXPR(loop_src_var),
                                  int_type, nullptr));
        } else if (ASR::is_a<ASR::Set_t>(*loop_src_var_ttype)) {
            throw SemanticError("Iterating on Set using for in loop not yet supported", loc);
        } else if (ASR::is_a<ASR::Tuple_t>(*loop_src_var_ttype)) {
            throw SemanticError("Iterating on Tuple using for in loop not yet supported", loc);
        } else {
            throw SemanticError("Only Strings, Lists, Sets and Tuples"
                                "can be used with for in loop, not " +
                                ASRUtils::type_to_str(loop_src_var_ttype), loc);
        }
        return nullptr;
    }

    ASR::do_loop_head_t make_do_loop_head(ASR::expr_t *loop_start, ASR::expr_t *loop_end,
                                            ASR::expr_t *inc, int a_kind, const Location& loc) {
        ASR::do_loop_head_t head;
        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, a_kind));
        ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, loc, 1, a_type));
        ASR::expr_t *constant_neg_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, loc, -1, a_type));
        ASR::expr_t *constant_zero = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, loc, 0, a_type));
        if (!loop_start) {
            loop_start = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, loc, 0, a_type));
        }
        if (!inc) {
            inc = constant_one;
        }

        if( !ASRUtils::check_equal_type(ASRUtils::expr_type(loop_start), ASRUtils::expr_type(loop_end)) ) {
            std::string loop_start_strtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(loop_start));
            std::string loop_end_strtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(loop_end));
            diag.add(diag::Diagnostic(
                "Type mismatch in loop start and loop end values, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + loop_start_strtype +
                                "' and '" + loop_end_strtype + "')",
                            {loop_start->base.loc, loop_end->base.loc})
                })
            );
            throw SemanticAbort();
        }

        if( !ASRUtils::check_equal_type(ASRUtils::expr_type(loop_start), ASRUtils::expr_type(inc)) ) {
            std::string loop_start_strtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(loop_start));
            std::string inc_strtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(inc));
            diag.add(diag::Diagnostic(
                "Type mismatch in loop start and increment values, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + loop_start_strtype +
                                "' and '" + inc_strtype + "')",
                            {loop_start->base.loc, inc->base.loc})
                })
            );
            throw SemanticAbort();
        }

        head.m_start = loop_start;
        head.m_increment = inc;

        if( !ASR::is_a<ASR::Integer_t>(*ASRUtils::type_get_past_const(
                ASRUtils::expr_type(inc))) ) {
            throw SemanticError("For loop increment type should be Integer.", loc);
        }
        ASR::expr_t *inc_value = ASRUtils::expr_value(inc);
        int64_t inc_int = 1;
        bool is_value_present = ASRUtils::extract_value(inc_value, inc_int);
        if (is_value_present) {
            if (inc_int == 0) {
                throw SemanticError("For loop increment should not be zero.", loc);
            }

            // Loop end depends upon the sign of m_increment.
            // if inc > 0 then: loop_end -=1 else loop_end += 1
            ASR::binopType offset_op;
            if (inc_int < 0 ) {
                offset_op = ASR::binopType::Add;
            } else {
                offset_op = ASR::binopType::Sub;
            }
            make_BinOp_helper(loop_end, constant_one,
                            offset_op, loc);
        } else {
            ASR::ttype_t* logical_type = ASRUtils::TYPE(ASR::make_Logical_t(al, inc->base.loc, 4));
            ASR::expr_t* inc_pos = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, inc->base.loc, inc,
                ASR::cmpopType::GtE, constant_zero, logical_type, nullptr));
            ASR::expr_t* inc_neg = ASRUtils::EXPR(ASR::make_IntegerCompare_t(al, inc->base.loc, inc,
                ASR::cmpopType::Lt, constant_zero, logical_type, nullptr));
            cast_helper(a_type, inc_pos, inc->base.loc, true);
            cast_helper(a_type, inc_neg, inc->base.loc, true);
            make_BinOp_helper(inc_pos, constant_neg_one, ASR::binopType::Mul, inc->base.loc);
            ASR::expr_t* case_1 = ASRUtils::EXPR(tmp);
            make_BinOp_helper(inc_neg, constant_one, ASR::binopType::Mul, inc->base.loc);
            ASR::expr_t* case_2 = ASRUtils::EXPR(tmp);
            make_BinOp_helper(case_1, case_2, ASR::binopType::Add, inc->base.loc);
            ASR::expr_t* cases_combined = ASRUtils::EXPR(tmp);
            make_BinOp_helper(loop_end, cases_combined, ASR::binopType::Add, loop_end->base.loc);
        }

        head.m_end = ASRUtils::EXPR(tmp);


        return head;
    }

    void visit_For(const AST::For_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target=ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        bool is_explicit_iterator_required = false;
        std::string explicit_iter_name = "";
        std::string loop_src_var_name = "";
        ASR::expr_t *loop_end = nullptr, *loop_start = nullptr, *inc = nullptr;
        ASR::expr_t *for_iter_type = nullptr;
        if (AST::is_a<AST::Call_t>(*x.m_iter)) {
            AST::Call_t *c = AST::down_cast<AST::Call_t>(x.m_iter);
            std::string call_name;
            if (AST::is_a<AST::Name_t>(*c->m_func)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(c->m_func);
                call_name = n->m_id;
            } else {
                throw SemanticError("Expected Name",
                    x.base.base.loc);
            }
            if (call_name != "range") {
                throw SemanticError(call_name + "(..) is not supported as for loop iteration for now",
                    x.base.base.loc);
            }
            Vec<ASR::expr_t*> args;
            args.reserve(al, c->n_args);
            for (size_t i=0; i<c->n_args; i++) {
                visit_expr(*c->m_args[i]);
                ASR::expr_t *expr = ASRUtils::EXPR(tmp);
                args.push_back(al, expr);
            }

            if (args.size() == 1) {
                loop_end = args[0];
            } else if (args.size() == 2) {
                loop_start = args[0];
                loop_end = args[1];
            } else if (args.size() == 3) {
                loop_start = args[0];
                loop_end = args[1];
                inc = args[2];
            } else {
                throw SemanticError("Only range(a, b, c) is supported as for loop iteration for now",
                    x.base.base.loc);
            }

        } else if (AST::is_a<AST::Name_t>(*x.m_iter)) {
            loop_src_var_name = AST::down_cast<AST::Name_t>(x.m_iter)->m_id;
            loop_end = for_iterable_helper(loop_src_var_name, x.base.base.loc, explicit_iter_name);
            for_iter_type = loop_end;
            LCOMPILERS_ASSERT(loop_end);
            is_explicit_iterator_required = true;
        } else if (AST::is_a<AST::Subscript_t>(*x.m_iter)) {
            AST::Subscript_t *sbt = AST::down_cast<AST::Subscript_t>(x.m_iter);
            if (AST::is_a<AST::Name_t>(*sbt->m_value)) {
                loop_src_var_name = AST::down_cast<AST::Name_t>(sbt->m_value)->m_id;
                visit_Subscript(*sbt);
                ASR::expr_t *target = ASRUtils::EXPR(tmp);
                ASR::symbol_t *loop_src_var_symbol = current_scope->resolve_symbol(loop_src_var_name);
                ASR::ttype_t *loop_src_var_ttype = ASRUtils::symbol_type(loop_src_var_symbol);

                // Create a temporary variable that will contain the evaluated value of Subscript
                std::string tmp_assign_name = current_scope->get_unique_name("__tmp_assign_for_loop", false);
                SetChar variable_dependencies_vec;
                variable_dependencies_vec.reserve(al, 1);
                ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, loop_src_var_ttype);
                ASR::asr_t* tmp_assign_variable = ASR::make_Variable_t(al, sbt->base.base.loc, current_scope,
                    s2c(al, tmp_assign_name), variable_dependencies_vec.p, variable_dependencies_vec.size(),
                    ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                    loop_src_var_ttype, nullptr, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false
                );
                ASR::symbol_t *tmp_assign_variable_sym = ASR::down_cast<ASR::symbol_t>(tmp_assign_variable);
                current_scope->add_symbol(tmp_assign_name, tmp_assign_variable_sym);

                // Assign the Subscript expr to temporary variable
                ASR::asr_t* assign = ASR::make_Assignment_t(al, x.base.base.loc,
                                ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, tmp_assign_variable_sym)),
                                target, nullptr);
                current_body->push_back(al, ASRUtils::STMT(assign));
                loop_end = for_iterable_helper(tmp_assign_name, x.base.base.loc, explicit_iter_name);
                for_iter_type = loop_end;
                LCOMPILERS_ASSERT(loop_end);
                loop_src_var_name = tmp_assign_name;
                is_explicit_iterator_required = true;
            } else {
                throw SemanticError("Only Name is supported for Subscript",
                    sbt->base.base.loc);
            }
        } else if (AST::is_a<AST::List_t>(*x.m_iter) || AST::is_a<AST::ConstantStr_t>(*x.m_iter) ) {
            visit_expr(*x.m_iter);
            ASR::expr_t *target = ASRUtils::EXPR(tmp);
            ASR::ttype_t *loop_src_var_ttype = ASRUtils::expr_type(target);

            // Create a temporary variable that will contain the evaluated value of List
            std::string tmp_assign_name = current_scope->get_unique_name("__tmp_assign_for_loop", false);
            SetChar variable_dependencies_vec;
            variable_dependencies_vec.reserve(al, 1);
            ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, loop_src_var_ttype);
            ASR::asr_t* tmp_assign_variable = ASR::make_Variable_t(al, target->base.loc, current_scope,
                s2c(al, tmp_assign_name), variable_dependencies_vec.p, variable_dependencies_vec.size(),
                ASR::intentType::Local, nullptr, nullptr, ASR::storage_typeType::Default,
                loop_src_var_ttype, nullptr, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false
            );
            ASR::symbol_t *tmp_assign_variable_sym = ASR::down_cast<ASR::symbol_t>(tmp_assign_variable);
            current_scope->add_symbol(tmp_assign_name, tmp_assign_variable_sym);

            // Assign the List expr to temporary variable
            ASR::asr_t* assign = ASR::make_Assignment_t(al, x.base.base.loc,
                            ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, tmp_assign_variable_sym)),
                            target, nullptr);
            current_body->push_back(al, ASRUtils::STMT(assign));
            loop_end = for_iterable_helper(tmp_assign_name, x.base.base.loc, explicit_iter_name);
            for_iter_type = loop_end;
            LCOMPILERS_ASSERT(loop_end);
            loop_src_var_name = tmp_assign_name;
            is_explicit_iterator_required = true;
        } else {
            throw SemanticError("Only function call `range(..)` supported as for loop iteration for now",
                x.base.base.loc);
        }
        int a_kind = 4;
        if (!is_explicit_iterator_required) {
            a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(target));
        }
        ASR::do_loop_head_t head = make_do_loop_head(loop_start, loop_end, inc, a_kind,
                                            x.base.base.loc);

        if (target) {
            ASR::Var_t* loop_var = ASR::down_cast<ASR::Var_t>(target);
            ASR::symbol_t* loop_var_sym = loop_var->m_v;
            do_loop_variables.push_back(loop_var_sym);
        }

        if(is_explicit_iterator_required) {
            body.reserve(al, x.n_body + 1);
            // add an assignment instruction to body to assign value of loop_src_var at an index to the loop_target_var
            LCOMPILERS_ASSERT(current_scope->get_symbol(explicit_iter_name) != nullptr);
            auto explicit_iter_var = ASR::make_Var_t(al, x.base.base.loc, current_scope->get_symbol(explicit_iter_name));
            ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, a_kind));
            ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                                al, x.base.base.loc, 1, a_type));
            auto index_plus_one = ASR::make_IntegerBinOp_t(al, x.base.base.loc, ASRUtils::EXPR(explicit_iter_var),
                ASR::binopType::Add, constant_one, a_type, nullptr);
            ASR::expr_t* loop_src_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, current_scope->resolve_symbol(loop_src_var_name)));
            ASR::ttype_t* loop_src_var_ttype = ASRUtils::expr_type(loop_src_var);
            ASR::asr_t* loop_src_var_element = nullptr;
            if (ASR::is_a<ASR::StringLen_t>(*for_iter_type)) {
                loop_src_var_element = ASR::make_StringItem_t(
                            al, x.base.base.loc, loop_src_var,
                            ASRUtils::EXPR(index_plus_one), ASRUtils::get_contained_type(loop_src_var_ttype), nullptr);
            } else if (ASR::is_a<ASR::ListLen_t>(*for_iter_type)) {
                loop_src_var_element = ASR::make_ListItem_t(
                            al, x.base.base.loc, loop_src_var,
                            ASRUtils::EXPR(explicit_iter_var), ASRUtils::get_contained_type(loop_src_var_ttype), nullptr);
            }
            auto loop_target_assignment = ASR::make_Assignment_t(al, x.base.base.loc, target, ASRUtils::EXPR(loop_src_var_element), nullptr);
            body.push_back(al, ASRUtils::STMT(loop_target_assignment));

            head.m_v = ASRUtils::EXPR(explicit_iter_var);
        } else {
            body.reserve(al, x.n_body);
            head.m_v = target;
        }

        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        current_scope->asr_owner = parent_scope->asr_owner;
        transform_stmts(body, x.n_body, x.m_body);
        int32_t total_syms = current_scope->get_scope().size();
        if( total_syms > 0 ) {
            std::string name = parent_scope->get_unique_name("block", false);
            ASR::asr_t* block = ASR::make_Block_t(al, x.base.base.loc,
                                                current_scope, s2c(al, name),
                                                body.p, body.size());
            current_scope = parent_scope;
            current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(block));
            ASR::stmt_t* decls = ASRUtils::STMT(ASR::make_BlockCall_t(al, x.base.base.loc,  -1,
                ASR::down_cast<ASR::symbol_t>(block)));
            body.reserve(al, 1);
            body.push_back(al, decls);
        }
        current_scope = parent_scope;
        head.loc = head.m_v->base.loc;
        bool parallel = false;
        if (x.m_type_comment) {
            if (std::string(x.m_type_comment) == "parallel") {
                parallel = true;
            }
        }
        if (parallel) {
            tmp = ASR::make_DoConcurrentLoop_t(al, x.base.base.loc, head,
                body.p, body.size());
        } else {
            tmp = ASR::make_DoLoop_t(al, x.base.base.loc, nullptr, head,
                body.p, body.size());
        }

        if (!do_loop_variables.empty()) {
            do_loop_variables.pop_back();
        }
    }

    void visit_AugAssign(const AST::AugAssign_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);
        ASR::ttype_t* left_type = ASRUtils::expr_type(left);
        ASR::ttype_t* right_type = ASRUtils::expr_type(right);
        ASR::binopType op = ASR::binopType::Add /* temporary assignment */;
        std::string op_name = "";
        switch (x.m_op) {
            case (AST::operatorType::Add) : { op = ASR::binopType::Add; break; }
            case (AST::operatorType::Sub) : { op = ASR::binopType::Sub; break; }
            case (AST::operatorType::Mult) : { op = ASR::binopType::Mul; break; }
            case (AST::operatorType::Div) : { op = ASR::binopType::Div; break; }
            case (AST::operatorType::Pow) : { op = ASR::binopType::Pow; break; }
            case (AST::operatorType::BitOr) : { op = ASR::binopType::BitOr; break; }
            case (AST::operatorType::BitAnd) : { op = ASR::binopType::BitAnd; break; }
            case (AST::operatorType::BitXor) : { op = ASR::binopType::BitXor; break; }
            case (AST::operatorType::LShift) : { op = ASR::binopType::BitLShift; break; }
            case (AST::operatorType::RShift) : { op = ASR::binopType::BitRShift; break; }
            case (AST::operatorType::Mod) : { op_name = "_mod"; break; }
            default : {
                throw SemanticError("Binary operator type not supported",
                    x.base.base.loc);
            }
        }

        cast_helper(left, right, false);

        if (!ASRUtils::check_equal_type(left_type, right_type)) {
            std::string ltype = ASRUtils::type_to_str_python(left_type);
            std::string rtype = ASRUtils::type_to_str_python(right_type);
            diag.add(diag::Diagnostic(
                "Type mismatch in shorthand operator, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }

        if (op_name != "") {
            Vec<ASR::call_arg_t> args;
            args.reserve(al, 2);
            ASR::call_arg_t arg1, arg2;
            arg1.loc = left->base.loc;
            arg1.m_value = left;
            args.push_back(al, arg1);
            arg2.loc = right->base.loc;
            arg2.m_value = right;
            args.push_back(al, arg2);
            ASR::symbol_t *fn_mod = resolve_intrinsic_function(x.base.base.loc, op_name);
            tmp = make_call_helper(al, fn_mod, current_scope, args, op_name, x.base.base.loc);
        } else {
            make_BinOp_helper(left, right, op, x.base.base.loc);
        }

        ASR::stmt_t* a_overloaded = nullptr;
        ASR::expr_t *tmp2 = ASR::down_cast<ASR::expr_t>(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, a_overloaded);

    }

    void visit_AttributeUtil(ASR::ttype_t* type, char* attr_char,
                             ASR::expr_t *e, const Location& loc) {
        if( ASR::is_a<ASR::Struct_t>(*type) ) {
            ASR::Struct_t* der = ASR::down_cast<ASR::Struct_t>(type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_derived_type);
            ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(der_sym);
            bool member_found = false;
            std::string member_name = attr_char;
            for( size_t i = 0; i < der_type->n_members && !member_found; i++ ) {
                member_found = std::string(der_type->m_members[i]) == member_name;
            }
            if( !member_found ) {
                throw SemanticError("No member " + member_name +
                                    " found in " + std::string(der_type->m_name),
                                    loc);
            }
            ASR::symbol_t* member_sym = der_type->m_symtab->resolve_symbol(member_name);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            ASR::ttype_t* member_var_type = member_var->m_type;
            if( ASR::is_a<ASR::Struct_t>(*member_var->m_type) ) {
                ASR::Struct_t* member_var_struct_t = ASR::down_cast<ASR::Struct_t>(member_var->m_type);
                if( !ASR::is_a<ASR::ExternalSymbol_t>(*member_var_struct_t->m_derived_type) ) {
                    ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(member_var_struct_t->m_derived_type);
                    ASR::symbol_t* struct_type_asr_owner = ASRUtils::get_asr_owner(member_var_struct_t->m_derived_type);
                    if( struct_type_asr_owner && ASR::is_a<ASR::StructType_t>(*struct_type_asr_owner) ) {
                        std::string struct_var_name = ASR::down_cast<ASR::StructType_t>(struct_type_asr_owner)->m_name;
                        std::string struct_member_name = struct_type->m_name;
                        std::string import_name = struct_var_name + "_" + struct_member_name;
                        ASR::symbol_t* import_struct_member = current_scope->resolve_symbol(import_name);
                        bool import_from_struct = true;
                        if( import_struct_member ) {
                            if( ASR::is_a<ASR::ExternalSymbol_t>(*import_struct_member) ) {
                                ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(import_struct_member);
                                if( ext_sym->m_external == member_var_struct_t->m_derived_type &&
                                    std::string(ext_sym->m_module_name) == struct_var_name ) {
                                    import_from_struct = false;
                                }
                            }
                        }
                        if( import_from_struct ) {
                            import_name = current_scope->get_unique_name(import_name, false);
                            import_struct_member = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                                                    loc, current_scope, s2c(al, import_name),
                                                                    member_var_struct_t->m_derived_type, s2c(al, struct_var_name), nullptr, 0,
                                                                    s2c(al, struct_member_name), ASR::accessType::Public));
                            current_scope->add_symbol(import_name, import_struct_member);
                        }
                        member_var_type = ASRUtils::TYPE(ASR::make_Struct_t(al, loc, import_struct_member));
                    }
                }
            }
            tmp = ASR::make_StructInstanceMember_t(al, loc, e, member_sym,
                                         member_var_type, nullptr);
        } else if(ASR::is_a<ASR::Enum_t>(*type)) {
             if( std::string(attr_char) == "value" ) {
                 ASR::Enum_t* enum_ = ASR::down_cast<ASR::Enum_t>(type);
                 ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_->m_enum_type);
                 tmp = ASR::make_EnumValue_t(al, loc, e, type, enum_type->m_type, nullptr);
             } else if( std::string(attr_char) == "name" ) {
                 ASR::ttype_t* char_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr));
                 tmp = ASR::make_EnumName_t(al, loc, e, type, char_type, nullptr);
             }
        } else if(ASR::is_a<ASR::Union_t>(*type)) {
             ASR::Union_t* u = ASR::down_cast<ASR::Union_t>(type);
             ASR::symbol_t* u_sym = ASRUtils::symbol_get_past_external(u->m_union_type);
             ASR::UnionType_t* u_type = ASR::down_cast<ASR::UnionType_t>(u_sym);
             bool member_found = false;
             std::string member_name = attr_char;
             for( size_t i = 0; i < u_type->n_members && !member_found; i++ ) {
                 member_found = std::string(u_type->m_members[i]) == member_name;
             }
             if( !member_found ) {
                 throw SemanticError("No member " + member_name +
                                     " found in " + std::string(u_type->m_name),
                                     loc);
             }
             ASR::symbol_t* member_sym = u_type->m_symtab->resolve_symbol(member_name);
             LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
             ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
             tmp = ASR::make_UnionInstanceMember_t(al, loc, e, member_sym,
                                          member_var->m_type, nullptr);
         } else if (ASRUtils::is_complex(*type)) {
            std::string attr = attr_char;
            int kind = ASRUtils::extract_kind_from_ttype_t(type);
            ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc, kind));
            if (attr == "imag") {
                tmp = ASR::make_ComplexIm_t(al, loc, e, dest_type, nullptr);
                return;
            } else if (attr == "real") {
                ASR::expr_t *value = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, e->base.loc, e, ASR::cast_kindType::ComplexToReal, dest_type));
                tmp = ASR::make_ComplexRe_t(al, loc, e, dest_type, ASRUtils::expr_value(value));
                return;
            } else {
                throw SemanticError("'" + attr + "' is not implemented for Complex type",
                    loc);
            }
        } else if( ASR::is_a<ASR::Pointer_t>(*type) ) {
            ASR::Pointer_t* ptr_type = ASR::down_cast<ASR::Pointer_t>(type);
            visit_AttributeUtil(ptr_type->m_type, attr_char, e, loc);
        } else {
            throw SemanticError(ASRUtils::type_to_str_python(type) + " not supported yet in Attribute.",
                loc);
        }
    }

    void visit_AttributeUtil(ASR::ttype_t* type, char* attr_char,
                             ASR::symbol_t *t, const Location& loc) {
        if (ASRUtils::is_array(type)) {
            std::string attr = attr_char;
            ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
            Vec<ASR::expr_t*> args;
            args.reserve(al, 0);
            handle_builtin_attribute(se, attr, loc, args);
        } else if (ASRUtils::is_complex(*type)) {
            std::string attr = attr_char;
            if (attr == "imag") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
                int kind = ASRUtils::extract_kind_from_ttype_t(type);
                ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc, kind));
                tmp = ASR::make_ComplexIm_t(al, loc, val, dest_type, nullptr);
                return;
            } else if (attr == "real") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
                int kind = ASRUtils::extract_kind_from_ttype_t(type);
                ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc, kind));
                ASR::expr_t *value = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, val->base.loc, val, ASR::cast_kindType::ComplexToReal, dest_type));
                tmp = ASR::make_ComplexRe_t(al, loc, val, dest_type, ASRUtils::expr_value(value));
                return;
            } else {
                throw SemanticError("'" + attr + "' is not implemented for Complex type",
                    loc);
            }
        } else if( ASR::is_a<ASR::Struct_t>(*type) ) {
            ASR::Struct_t* der = ASR::down_cast<ASR::Struct_t>(type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_derived_type);
            ASR::StructType_t* der_type = ASR::down_cast<ASR::StructType_t>(der_sym);
            bool member_found = false;
            std::string member_name = attr_char;
            for( size_t i = 0; i < der_type->n_members && !member_found; i++ ) {
                member_found = std::string(der_type->m_members[i]) == member_name;
            }
            if( !member_found ) {
                throw SemanticError("No member " + member_name +
                                    " found in " + std::string(der_type->m_name),
                                    loc);
            }
            ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
            ASR::symbol_t* member_sym = der_type->m_symtab->resolve_symbol(member_name);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            ASR::ttype_t* member_var_type = member_var->m_type;
            if( ASR::is_a<ASR::Struct_t>(*member_var->m_type) ) {
                ASR::Struct_t* member_var_struct_t = ASR::down_cast<ASR::Struct_t>(member_var->m_type);
                if( !ASR::is_a<ASR::ExternalSymbol_t>(*member_var_struct_t->m_derived_type) ) {
                    ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(member_var_struct_t->m_derived_type);
                    ASR::symbol_t* struct_type_asr_owner = ASRUtils::get_asr_owner(member_var_struct_t->m_derived_type);
                    if( struct_type_asr_owner && ASR::is_a<ASR::StructType_t>(*struct_type_asr_owner) ) {
                        std::string struct_var_name = ASR::down_cast<ASR::StructType_t>(struct_type_asr_owner)->m_name;
                        std::string struct_member_name = struct_type->m_name;
                        std::string import_name = struct_var_name + "_" + struct_member_name;
                        ASR::symbol_t* import_struct_member = current_scope->resolve_symbol(import_name);
                        bool import_from_struct = true;
                        if( import_struct_member ) {
                            if( ASR::is_a<ASR::ExternalSymbol_t>(*import_struct_member) ) {
                                ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(import_struct_member);
                                if( ext_sym->m_external == member_var_struct_t->m_derived_type &&
                                    std::string(ext_sym->m_module_name) == struct_var_name ) {
                                    import_from_struct = false;
                                }
                            }
                        }
                        if( import_from_struct ) {
                            import_name = current_scope->get_unique_name(import_name, false);
                            import_struct_member = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                                                    loc, current_scope, s2c(al, import_name),
                                                                    member_var_struct_t->m_derived_type, s2c(al, struct_var_name), nullptr, 0,
                                                                    s2c(al, struct_member_name), ASR::accessType::Public));
                            current_scope->add_symbol(import_name, import_struct_member);
                        }
                        member_var_type = ASRUtils::TYPE(ASR::make_Struct_t(al, loc, import_struct_member));
                    }
                }
            }
            tmp = ASR::make_StructInstanceMember_t(al, loc, val, member_sym,
                                            member_var_type, nullptr);
        } else if (ASR::is_a<ASR::Enum_t>(*type)) {
            ASR::Enum_t* enum_ = ASR::down_cast<ASR::Enum_t>(type);
            ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(enum_->m_enum_type);
            std::string attr_name = attr_char;
            if( attr_name != "value" && attr_name != "name" ) {
                throw SemanticError(attr_name + " property not yet supported with Enums. "
                    "Only value and name properties are supported for now.", loc);
            }
            ASR::Variable_t* t_var = ASR::down_cast<ASR::Variable_t>(t);
            ASR::expr_t* t_mem = nullptr;
            if( enum_type->m_symtab->get_symbol(std::string(t_var->m_name)) == t ) {
                ASR::expr_t* enum_type_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, enum_->m_enum_type));
                t_mem = ASRUtils::EXPR(ASR::make_EnumStaticMember_t(al, loc, enum_type_var, t,
                                        enum_type->m_type, ASRUtils::expr_value(t_var->m_symbolic_value)));
            } else {
                t_mem = ASRUtils::EXPR(ASR::make_Var_t(al, loc, t));
            }
            if( attr_name == "value" ) {
                tmp = ASR::make_EnumValue_t(al, loc, t_mem, type, enum_type->m_type, nullptr);
            } else if( attr_name == "name" ) {
                ASR::ttype_t* char_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc, 1, -2, nullptr));
                tmp = ASR::make_EnumName_t(al, loc, t_mem, type, char_type, nullptr);
            }
        } else if (ASR::is_a<ASR::Union_t>(*type)) {
            ASR::Union_t* union_asr = ASR::down_cast<ASR::Union_t>(type);
            ASR::symbol_t* union_sym = ASRUtils::symbol_get_past_external(union_asr->m_union_type);
            ASR::UnionType_t* union_type = ASR::down_cast<ASR::UnionType_t>(union_sym);
            bool member_found = false;
            std::string member_name = attr_char;
            for( size_t i = 0; i < union_type->n_members && !member_found; i++ ) {
                member_found = std::string(union_type->m_members[i]) == member_name;
            }
            if( !member_found ) {
                throw SemanticError("No member " + member_name +
                                    " found in " + std::string(union_type->m_name),
                                    loc);
            }
            ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
            ASR::symbol_t* member_sym = union_type->m_symtab->resolve_symbol(member_name);
            LCOMPILERS_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            tmp = ASR::make_UnionInstanceMember_t(al, loc, val, member_sym,
                                            member_var->m_type, nullptr);
        } else if(ASR::is_a<ASR::Pointer_t>(*type)) {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(type);
            visit_AttributeUtil(p->m_type, attr_char, t, loc);
        } else if(ASR::is_a<ASR::SymbolicExpression_t>(*type)) {
            std::string attr = attr_char;
            if (attr == "func") {
                using_func_attr = true;
                return;
            }
            if (attr == "args") {
                using_args_attr = true;
                return;
            }
            ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
            Vec<ASR::expr_t*> args;
            args.reserve(al, 0);
            handle_symbolic_attribute(se, attr, loc, args);
        } else {
            throw SemanticError(ASRUtils::type_to_str_python(type) + " not supported yet in Attribute.",
                loc);
        }
    }

    void visit_Attribute(const AST::Attribute_t &x) {
        if (AST::is_a<AST::Name_t>(*x.m_value)) {
            std::string value = AST::down_cast<AST::Name_t>(x.m_value)->m_id;

            ASR::symbol_t *org_sym = current_scope->resolve_symbol(value);
            if (!org_sym) {
                throw SemanticError("'" + value + "' is not defined in the scope",
                    x.base.base.loc);
            }
            ASR::symbol_t *t = ASRUtils::symbol_get_past_external(org_sym);
            if (ASR::is_a<ASR::Variable_t>(*t)) {
                ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(t);
                visit_AttributeUtil(var->m_type, x.m_attr, t, x.base.base.loc);
            } else if (ASR::is_a<ASR::EnumType_t>(*t)) {
                ASR::EnumType_t* enum_type = ASR::down_cast<ASR::EnumType_t>(t);
                ASR::symbol_t* enum_member = enum_type->m_symtab->resolve_symbol(std::string(x.m_attr));
                if( !enum_member ) {
                    throw SemanticError(std::string(x.m_attr) + " not present in " +
                                        std::string(enum_type->m_name) + " enum.",
                                        x.base.base.loc);
                }
                ASR::Variable_t* enum_member_variable = ASR::down_cast<ASR::Variable_t>(enum_member);
                ASR::expr_t* enum_type_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, org_sym));
                ASR::expr_t* enum_member_var = ASRUtils::EXPR(ASR::make_EnumStaticMember_t(al, x.base.base.loc, enum_type_var,
                                                    enum_member, enum_type->m_type,
                                                    ASRUtils::expr_value(enum_member_variable->m_symbolic_value)));
                ASR::ttype_t* enum_t = ASRUtils::TYPE(ASR::make_Enum_t(al, x.base.base.loc, t));
                tmp = ASR::make_EnumValue_t(al, x.base.base.loc, enum_member_var,
                        enum_t, enum_member_variable->m_type,
                        ASRUtils::expr_value(enum_member_variable->m_symbolic_value));
            } else if (ASR::is_a<ASR::StructType_t>(*t)) {
                ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(t);
                ASR::symbol_t* struct_member = struct_type->m_symtab->resolve_symbol(std::string(x.m_attr));
                if( !struct_member ) {
                    throw SemanticError(std::string(x.m_attr) + " not present in " +
                                        std::string(struct_type->m_name) + " dataclass.",
                                        x.base.base.loc);
                }
                if( ASR::is_a<ASR::Variable_t>(*struct_member) ) {
                    ASR::Variable_t* struct_member_variable = ASR::down_cast<ASR::Variable_t>(struct_member);
                    ASR::expr_t* struct_type_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, org_sym));
                    tmp = ASR::make_StructStaticMember_t(al, x.base.base.loc,
                                                        struct_type_var, struct_member, struct_member_variable->m_type,
                                                        nullptr);
                } else if( ASR::is_a<ASR::UnionType_t>(*struct_member) ) {
                    ASR::expr_t* struct_type_var = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, org_sym));
                    ASR::ttype_t* union_type = ASRUtils::TYPE(ASR::make_Union_t(al, x.base.base.loc, struct_member));
                    tmp = ASR::make_StructStaticMember_t(al, x.base.base.loc, struct_type_var, struct_member, union_type, nullptr);
                }
            } else if (ASR::is_a<ASR::Module_t>(*t)) {
                ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
                std::string sym_name = value + "@" + x.m_attr;
                ASR::symbol_t *sym = current_scope->resolve_symbol(sym_name);
                if (!sym) {
                    sym = import_from_module(al, m, current_scope, value,
                                    x.m_attr, sym_name, x.base.base.loc, false);
                    LCOMPILERS_ASSERT(ASR::is_a<ASR::ExternalSymbol_t>(*sym));
                    current_scope->add_symbol(sym_name, sym);
                }
                tmp = ASR::make_Var_t(al, x.base.base.loc, sym);
            } else {
                throw SemanticError("Only Variable type is supported for now in Attribute",
                    x.base.base.loc);
            }

        } else if(AST::is_a<AST::Attribute_t>(*x.m_value)) {
            AST::Attribute_t* x_m_value = AST::down_cast<AST::Attribute_t>(x.m_value);
            visit_Attribute(*x_m_value);
            ASR::expr_t* e = ASRUtils::EXPR(tmp);
            if( ASR::is_a<ASR::EnumValue_t>(*e) ) {
                std::string enum_property = std::string(x.m_attr);
                if( enum_property != "value" &&
                    enum_property != "name" ) {
                    throw SemanticError(enum_property + " is not a supported property of enum member."
                                        " Only value and name are supported for now.",
                                        x.base.base.loc);
                }
                ASR::EnumValue_t* enum_ref = ASR::down_cast<ASR::EnumValue_t>(e);
                if( enum_property == "value" ) {
                    ASR::expr_t* enum_ref_value = ASRUtils::expr_value(enum_ref->m_v);
                    tmp = ASR::make_EnumValue_t(al, x.base.base.loc, enum_ref->m_v,
                                enum_ref->m_enum_type, enum_ref->m_type,
                                enum_ref_value);
                } else if( enum_property == "name" ) {
                    ASR::expr_t* enum_ref_value = nullptr;
                    ASR::ttype_t* enum_ref_type = enum_ref->m_type;
                    if( ASR::is_a<ASR::EnumStaticMember_t>(*enum_ref->m_v) ) {
                        ASR::EnumStaticMember_t* enum_Var = ASR::down_cast<ASR::EnumStaticMember_t>(enum_ref->m_v);
                        ASR::Variable_t* enum_m_var = ASR::down_cast<ASR::Variable_t>(enum_Var->m_m);
                        char *s = enum_m_var->m_name;
                        size_t s_size = std::string(s).size();
                        enum_ref_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc, 1, s_size, nullptr));
                        enum_ref_value = ASRUtils::EXPR(ASR::make_StringConstant_t(al, x.base.base.loc,
                                            s, enum_ref_type));
                    }
                    tmp = ASR::make_EnumName_t(al, x.base.base.loc, enum_ref->m_v, enum_ref->m_enum_type,
                                 enum_ref_type, enum_ref_value);
                }
            } else {
                visit_AttributeUtil(ASRUtils::expr_type(e), x.m_attr, e, x.base.base.loc);
            }
        } else if(AST::is_a<AST::Subscript_t>(*x.m_value)) {
            AST::Subscript_t* x_m_value = AST::down_cast<AST::Subscript_t>(x.m_value);
            visit_Subscript(*x_m_value);
            ASR::expr_t* e = ASRUtils::EXPR(tmp);
            visit_AttributeUtil(ASRUtils::expr_type(e), x.m_attr, e, x.base.base.loc);
        } else {
            throw SemanticError("Only Name, Attribute is supported for now in Attribute",
                x.base.base.loc);
        }
    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        transform_stmts(orelse, x.n_orelse, x.m_orelse);
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_Dict(const AST::Dict_t &x) {
        LCOMPILERS_ASSERT(x.n_keys == x.n_values);
        if( x.n_keys == 0 ) {
            if( assign_asr_target != nullptr ) {
                tmp = ASR::make_DictConstant_t(al, x.base.base.loc, nullptr, 0,
                                            nullptr, 0, ASRUtils::expr_type(assign_asr_target));
            }
            else {
                tmp = nullptr;
            }
            return ;
        }
        Vec<ASR::expr_t*> keys;
        keys.reserve(al, x.n_keys);
        ASR::ttype_t* key_type = nullptr;
        for (size_t i = 0; i < x.n_keys; ++i) {
            visit_expr(*x.m_keys[i]);
            ASR::expr_t *key = ASRUtils::EXPR(tmp);
            if (key_type == nullptr) {
                key_type = ASRUtils::expr_type(key);
            } else {
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(key), key_type)) {
                    throw SemanticError("All dictionary keys must be of the same type",
                                        x.base.base.loc);
                }
            }
            keys.push_back(al, key);
        }
        Vec<ASR::expr_t*> values;
        values.reserve(al, x.n_values);
        ASR::ttype_t* value_type = nullptr;
        for (size_t i = 0; i < x.n_values; ++i) {
            visit_expr(*x.m_values[i]);
            ASR::expr_t *value = ASRUtils::EXPR(tmp);
            if (value_type == nullptr) {
                value_type = ASRUtils::expr_type(value);
            } else {
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(value), value_type)) {
                    throw SemanticError("All dictionary values must be of the same type",
                                        x.base.base.loc);
                }
            }
            values.push_back(al, value);
        }
        raise_error_when_dict_key_is_float_or_complex(key_type, x.base.base.loc);
        ASR::ttype_t* type = ASRUtils::TYPE(ASR::make_Dict_t(al, x.base.base.loc,
                                             key_type, value_type));
        tmp = ASR::make_DictConstant_t(al, x.base.base.loc, keys.p, keys.size(),
                                             values.p, values.size(), type);
    }

    void visit_While(const AST::While_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, nullptr, test, body.p,
                body.size());
    }

    void visit_Compare(const AST::Compare_t &x) {
        if (x.n_comparators > 1) {
            diag.add(diag::Diagnostic(
                "Only one comparison operator is supported for now",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("multiple comparison operators",
                            {x.m_comparators[0]->base.loc})
                })
            );
            throw SemanticAbort();
        }
        this->visit_expr(*x.m_left);
        if (using_func_attr) {
            if (AST::is_a<AST::Attribute_t>(*x.m_left) && AST::is_a<AST::Name_t>(*x.m_comparators[0])) {
                AST::Attribute_t *attr = AST::down_cast<AST::Attribute_t>(x.m_left);
                AST::Name_t *type_name = AST::down_cast<AST::Name_t>(x.m_comparators[0]);
                std::string symbolic_type = type_name->m_id;
                if (AST::is_a<AST::Name_t>(*attr->m_value)) {
                    AST::Name_t *var_name = AST::down_cast<AST::Name_t>(attr->m_value);
                    std::string var = var_name->m_id;
                    ASR::symbol_t *st = current_scope->resolve_symbol(var);
                    ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(
                                    ASR::make_Var_t(al, x.base.base.loc, st));
                    Vec<ASR::expr_t*> args;
                    args.reserve(al, 0);
                    if (symbolic_type == "Add") {
                        tmp = attr_handler.eval_symbolic_is_Add(se, al, x.base.base.loc, args, diag);
                        return;
                    } else if (symbolic_type == "Mul") {
                        tmp = attr_handler.eval_symbolic_is_Mul(se, al, x.base.base.loc, args, diag);
                        return;
                    } else if (symbolic_type == "Pow") {
                        tmp = attr_handler.eval_symbolic_is_Pow(se, al, x.base.base.loc, args, diag);
                        return;
                    } else if (symbolic_type == "log") {
                        tmp = attr_handler.eval_symbolic_is_log(se, al, x.base.base.loc, args, diag);
                        return;
                    } else if (symbolic_type == "sin") {
                        tmp = attr_handler.eval_symbolic_is_sin(se, al, x.base.base.loc, args, diag);
                        return;
                    } else {
                        throw SemanticError(symbolic_type + " symbolic type not supported yet", x.base.base.loc);
                    }
                }
            }
        }
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_comparators[0]);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);

        ASR::cmpopType asr_op;
        switch (x.m_ops) {
            case (AST::cmpopType::Eq): { asr_op = ASR::cmpopType::Eq; break; }
            case (AST::cmpopType::Gt): { asr_op = ASR::cmpopType::Gt; break; }
            case (AST::cmpopType::GtE): { asr_op = ASR::cmpopType::GtE; break; }
            case (AST::cmpopType::Lt): { asr_op = ASR::cmpopType::Lt; break; }
            case (AST::cmpopType::LtE): { asr_op = ASR::cmpopType::LtE; break; }
            case (AST::cmpopType::NotEq): { asr_op = ASR::cmpopType::NotEq; break; }
            default: {
                throw SemanticError("Comparison operator not implemented",
                                    x.base.base.loc);
            }
        }

        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        if( ASR::is_a<ASR::Const_t>(*left_type) ) {
            left_type = ASRUtils::get_contained_type(left_type);
        }
        if( ASR::is_a<ASR::Const_t>(*right_type) ) {
            right_type = ASRUtils::get_contained_type(right_type);
        }
        ASR::expr_t *overloaded = nullptr;

        if (!ASRUtils::is_logical(*left_type) || !ASRUtils::is_logical(*right_type)) {
            cast_helper(left, right, false);
        }

        left_type = ASRUtils::expr_type(left);
        right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = left_type;
        if (!ASRUtils::check_equal_type(left_type, right_type)) {
            std::string ltype = ASRUtils::type_to_str_python(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str_python(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Type mismatch in comparison operator, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch ('" + ltype + "' and '" + rtype + "')",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }
        ASR::ttype_t *type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4));
        ASR::expr_t *value = nullptr;

        if( ASR::is_a<ASR::Enum_t>(*dest_type) || ASR::is_a<ASR::Const_t>(*dest_type) ) {
            dest_type = ASRUtils::get_contained_type(dest_type);
        }

        if (ASRUtils::is_array(dest_type)) {
            ASR::dimension_t* m_dims = nullptr;
            int n_dims = ASRUtils::extract_dimensions_from_ttype(dest_type, m_dims);
            int array_size = ASRUtils::get_fixed_size_of_array(m_dims, n_dims);
            if (array_size == -1) {
                throw SemanticError("The truth value of an array is ambiguous. Use a.any() or a.all()", x.base.base.loc);
            } else if (array_size != 1) {
                throw SemanticError("The truth value of an array with more than one element is ambiguous. Use a.any() or a.all()", x.base.base.loc);
            } else {
                Vec<ASR::array_index_t> argsL, argsR;
                argsL.reserve(al, 1);
                argsR.reserve(al, 1);
                for (int i = 0; i < n_dims; i++) {
                    ASR::array_index_t aiL, aiR;
                    ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
                    ASR::expr_t* const_zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, x.base.base.loc, 0, int_type));
                    aiL.m_right = aiR.m_right = const_zero;
                    aiL.m_left = aiR.m_left = nullptr;
                    aiL.m_step = aiR.m_step = nullptr;
                    aiL.loc = left->base.loc;
                    aiR.loc = right->base.loc;
                    argsL.push_back(al, aiL);
                    argsR.push_back(al, aiR);
                }
                dest_type = ASRUtils::type_get_past_array(dest_type);
                left = ASRUtils::EXPR(make_ArrayItem_t(al, left->base.loc, left, argsL.p, argsL.n, dest_type, ASR::arraystorageType::RowMajor, nullptr));
                right = ASRUtils::EXPR(make_ArrayItem_t(al, right->base.loc, right, argsR.p, argsR.n, dest_type, ASR::arraystorageType::RowMajor, nullptr));
            }
        }

        if (ASRUtils::is_integer(*dest_type)) {
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = -1;
                ASRUtils::extract_value(ASRUtils::expr_value(left), left_value);
                int64_t right_value = -1;
                ASRUtils::extract_value(ASRUtils::expr_value(right), right_value);
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq):  { result = left_value == right_value; break; }
                    case (ASR::cmpopType::Gt): { result = left_value > right_value; break; }
                    case (ASR::cmpopType::GtE): { result = left_value >= right_value; break; }
                    case (ASR::cmpopType::Lt): { result = left_value < right_value; break; }
                    case (ASR::cmpopType::LtE): { result = left_value <= right_value; break; }
                    case (ASR::cmpopType::NotEq): { result = left_value != right_value; break; }
                    default: {
                        throw SemanticError("Comparison operator not implemented",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }
            tmp = ASR::make_IntegerCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASRUtils::is_unsigned_integer(*dest_type)) {
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = -1;
                ASRUtils::extract_value(ASRUtils::expr_value(left), left_value);
                int64_t right_value = -1;
                ASRUtils::extract_value(ASRUtils::expr_value(right), right_value);
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq):  { result = left_value == right_value; break; }
                    case (ASR::cmpopType::Gt): { result = left_value > right_value; break; }
                    case (ASR::cmpopType::GtE): { result = left_value >= right_value; break; }
                    case (ASR::cmpopType::Lt): { result = left_value < right_value; break; }
                    case (ASR::cmpopType::LtE): { result = left_value <= right_value; break; }
                    case (ASR::cmpopType::NotEq): { result = left_value != right_value; break; }
                    default: {
                        throw SemanticError("Comparison operator not implemented",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }
            tmp = ASR::make_UnsignedIntegerCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASRUtils::is_real(*dest_type)) {

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                double left_value = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::RealConstant_t>(
                                        ASRUtils::expr_value(right))->m_r;
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq):  { result = left_value == right_value; break; }
                    case (ASR::cmpopType::Gt): { result = left_value > right_value; break; }
                    case (ASR::cmpopType::GtE): { result = left_value >= right_value; break; }
                    case (ASR::cmpopType::Lt): { result = left_value < right_value; break; }
                    case (ASR::cmpopType::LtE): { result = left_value <= right_value; break; }
                    case (ASR::cmpopType::NotEq): { result = left_value != right_value; break; }
                    default: {
                        throw SemanticError("Comparison operator not implemented",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }

            tmp = ASR::make_RealCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);

        } else if (ASRUtils::is_complex(*dest_type)) {

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                ASR::ComplexConstant_t *left0
                    = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(left));
                ASR::ComplexConstant_t *right0
                    = ASR::down_cast<ASR::ComplexConstant_t>(ASRUtils::expr_value(right));
                std::complex<double> left_value(left0->m_re, left0->m_im);
                std::complex<double> right_value(right0->m_re, right0->m_im);
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq) : {
                        result = left_value.real() == right_value.real() &&
                                left_value.imag() == right_value.imag();
                        break;
                    }
                    case (ASR::cmpopType::NotEq) : {
                        result = left_value.real() != right_value.real() ||
                                left_value.imag() != right_value.imag();
                        break;
                    }
                    default: {
                        throw SemanticError("'" + ASRUtils::cmpop_to_str(asr_op) +
                                            "' comparison is not supported between complex numbers",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }

            tmp = ASR::make_ComplexCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);

        } else if (ASRUtils::is_logical(*dest_type)) {

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                bool left_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                        ASRUtils::expr_value(left))->m_value;
                bool right_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                        ASRUtils::expr_value(right))->m_value;
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq):  { result = left_value == right_value; break; }
                    case (ASR::cmpopType::Gt): { result = left_value > right_value; break; }
                    case (ASR::cmpopType::GtE): { result = left_value >= right_value; break; }
                    case (ASR::cmpopType::Lt): { result = left_value < right_value; break; }
                    case (ASR::cmpopType::LtE): { result = left_value <= right_value; break; }
                    case (ASR::cmpopType::NotEq): { result = left_value != right_value; break; }
                    default: {
                        throw SemanticError("Comparison operator not implemented",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }

            tmp = ASR::make_LogicalCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);

        } else if (ASRUtils::is_character(*dest_type)) {

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* left_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(right))->m_s;
                std::string left_str = std::string(left_value);
                std::string right_str = std::string(right_value);
                int8_t strcmp = left_str.compare(right_str);
                bool result;
                switch (asr_op) {
                    case (ASR::cmpopType::Eq) : {
                        result = (strcmp == 0);
                        break;
                    }
                    case (ASR::cmpopType::NotEq) : {
                        result = (strcmp != 0);
                        break;
                    }
                    case (ASR::cmpopType::Gt) : {
                        result = (strcmp > 0);
                        break;
                    }
                    case (ASR::cmpopType::GtE) : {
                        result = (strcmp > 0 || strcmp == 0);
                        break;
                    }
                    case (ASR::cmpopType::Lt) : {
                        result = (strcmp < 0);
                        break;
                    }
                    case (ASR::cmpopType::LtE) : {
                        result = (strcmp < 0 || strcmp == 0);
                        break;
                    }
                    default: {
                        throw SemanticError("ICE: Unknown compare operator", x.base.base.loc); // should never happen
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }

            tmp = ASR::make_StringCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASR::is_a<ASR::Tuple_t>(*dest_type)) {
            if (asr_op != ASR::cmpopType::Eq && asr_op != ASR::cmpopType::NotEq
                && asr_op != ASR::cmpopType::Lt && asr_op != ASR::cmpopType::LtE
                && asr_op != ASR::cmpopType::Gt && asr_op != ASR::cmpopType::GtE) {
                throw SemanticError("Only ==, !=, <, <=, >, >= operators "
                                    "are supported for Tuples",
                                x.base.base.loc);
            }
            tmp = ASR::make_TupleCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASR::is_a<ASR::List_t>(*dest_type)) {
            if (asr_op != ASR::cmpopType::Eq && asr_op != ASR::cmpopType::NotEq
                && asr_op != ASR::cmpopType::Lt && asr_op != ASR::cmpopType::LtE
                && asr_op != ASR::cmpopType::Gt && asr_op != ASR::cmpopType::GtE) {
                throw SemanticError("Only ==, !=, <, <=, >, >= operators "
                                    "are supported for Lists",
                                x.base.base.loc);
            }
            tmp = ASR::make_ListCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASR::is_a<ASR::CPtr_t>(*dest_type)) {
            if (asr_op != ASR::cmpopType::Eq && asr_op != ASR::cmpopType::NotEq) {
                throw SemanticError("Only Equal and Not-equal operators are supported for CPtr",
                                x.base.base.loc);
            }
            tmp = ASR::make_CPtrCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else if (ASR::is_a<ASR::SymbolicExpression_t>(*dest_type)) {
            tmp = ASR::make_SymbolicCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        } else {
            throw SemanticError("Compare not supported for type: " + ASRUtils::type_to_str_python(dest_type),
                                x.base.base.loc);
        }

        if (overloaded != nullptr) {
            tmp = ASR::make_OverloadedCompare_t(al, x.base.base.loc, left, asr_op, right, type,
                value, overloaded);
        }
    }

    void visit_ConstantEllipsis(const AST::ConstantEllipsis_t &/*x*/) {
        tmp = nullptr;
    }

    void visit_Pass(const AST::Pass_t &/*x*/) {
        tmp = nullptr;
    }

    void visit_Return(const AST::Return_t &x) {
        std::string return_var_name = "_lpython_return_variable";
        ASR::symbol_t *return_var = current_scope->resolve_symbol(return_var_name);
        if(!return_var) {
            if (x.m_value) {
                throw SemanticError("Return type of function is not defined",
                                x.base.base.loc);
            }
            // this may be a case with void return type (like subroutine)
            tmp = ASR::make_Return_t(al, x.base.base.loc);
            return;
        }
        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc, return_var);
        ASR::expr_t *target = ASRUtils::EXPR(return_var_ref);
        ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        if( ASR::is_a<ASR::Const_t>(*target_type) ) {
            target_type = ASRUtils::get_contained_type(target_type);
        }
        ASR::expr_t* assign_asr_target_copy = assign_asr_target;
        assign_asr_target = target;
        this->visit_expr(*x.m_value);
        assign_asr_target = assign_asr_target_copy;
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
        if( ASR::is_a<ASR::Const_t>(*value_type) ) {
            value_type = ASRUtils::get_contained_type(value_type);
        }
        if (!ASRUtils::check_equal_type(target_type, value_type)) {
            std::string ltype = ASRUtils::type_to_str_python(target_type);
            std::string rtype = ASRUtils::type_to_str_python(value_type);
            throw SemanticError("Type Mismatch in return, found ('" +
                    ltype + "' and '" + rtype + "')", x.base.base.loc);
        }
        cast_helper(target, value, true);
        ASR::stmt_t *overloaded=nullptr;
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                overloaded);
        // if( ASR::is_a<ASR::Const_t>(*ASRUtils::symbol_type(return_var)) ) {
        //     ASR::Variable_t* return_variable = ASR::down_cast<ASR::Variable_t>(return_var);
        //     return_variable->m_symbolic_value = value;
        //     SetChar variable_dependencies_vec;
        //     variable_dependencies_vec.from_pointer_n_copy(al, return_variable->m_dependencies,
        //                                                   return_variable->n_dependencies);
        //     ASRUtils::collect_variable_dependencies(al, variable_dependencies_vec, nullptr, value, nullptr);
        //     return_variable->m_dependencies = variable_dependencies_vec.p;
        //     return_variable->n_dependencies = variable_dependencies_vec.size();
        // }

        // We can only return one statement in `tmp`, so we insert the current
        // `tmp` into the body of the function directly
        current_body->push_back(al, ASR::down_cast<ASR::stmt_t>(tmp));

        // Now we assign Return into `tmp`
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    void visit_Continue(const AST::Continue_t &x) {
        tmp = ASR::make_Cycle_t(al, x.base.base.loc, nullptr);
    }

    void visit_Break(const AST::Break_t &x) {
        tmp = ASR::make_Exit_t(al, x.base.base.loc, nullptr);
    }

    void visit_Raise(const AST::Raise_t &x) {
        ASR::expr_t *code;
        if (x.m_cause) {
            visit_expr(*x.m_cause);
            code = ASRUtils::EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_ErrorStop_t(al, x.base.base.loc, code);
    }

    void visit_Set(const AST::Set_t &x) {
        LCOMPILERS_ASSERT(x.n_elts > 0); // type({}) is 'dict'
        Vec<ASR::expr_t*> elements;
        elements.reserve(al, x.n_elts);
        ASR::ttype_t* type = nullptr;
        for (size_t i = 0; i < x.n_elts; ++i) {
            visit_expr(*x.m_elts[i]);
            ASR::expr_t *value = ASRUtils::EXPR(tmp);
            if (type == nullptr) {
                type = ASRUtils::expr_type(value);
            } else {
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(value), type)) {
                    throw SemanticError("All Set values must be of the same type for now",
                                        x.base.base.loc);
                }
            }
            elements.push_back(al, value);
        }
        ASR::ttype_t* set_type = ASRUtils::TYPE(ASR::make_Set_t(al, x.base.base.loc, type));
        tmp = ASR::make_SetConstant_t(al, x.base.base.loc, elements.p, elements.size(), set_type);
    }

    void visit_Expr(const AST::Expr_t &x) {
        if (AST::is_a<AST::Call_t>(*x.m_value)) {
            AST::Call_t *c = AST::down_cast<AST::Call_t>(x.m_value);
            visit_Call(*c);
            return;
        }
        this->visit_expr(*x.m_value);

        // If tmp is a statement and not an expression
        // never cast into expression using ASRUtils::EXPR
        // Just ignore and exit the function naturally.
        if( tmp && !ASR::is_a<ASR::stmt_t>(*tmp) ) {
            LCOMPILERS_ASSERT(ASR::is_a<ASR::expr_t>(*tmp));
            tmp = nullptr;
        }
    }

    ASR::asr_t* create_PointerToCPtr(const AST::Call_t& x) {
        if( x.n_args != 2 ) {
            throw SemanticError("p_c_pointer accepts two positional arguments, "
                                "first a Pointer variable, second a CPtr variable.",
                                x.base.base.loc);
        }
        visit_expr(*x.m_args[0]);
        ASR::expr_t* pptr = ASRUtils::EXPR(tmp);
        visit_expr(*x.m_args[1]);
        ASR::expr_t* cptr = ASRUtils::EXPR(tmp);
        ASR::asr_t* pp = ASR::make_PointerToCPtr_t(al, x.base.base.loc, pptr,
                                         ASRUtils::expr_type(cptr), nullptr);
        return ASR::make_Assignment_t(al, x.base.base.loc,
            cptr, ASR::down_cast<ASR::expr_t>(pp), nullptr);
    }

    void handle_string_attributes(ASR::expr_t *s_var,
                Vec<ASR::call_arg_t> &args, std::string attr_name, const Location &loc) {
        std::string fn_call_name = "";
        Vec<ASR::call_arg_t> fn_args;
        fn_args.reserve(al, 1);
        if (attr_name == "capitalize") {
            if (args.size() != 0) {
                throw SemanticError("str.capitalize() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_capitalize";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "lower") {
            if (args.size() != 0) {
                throw SemanticError("str.lower() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_lower";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "isalpha") {
            if (args.size() != 0) {
                throw SemanticError("str.isalpha() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_isalpha";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "istitle") {
            if (args.size() != 0) {
                throw SemanticError("str.istitle() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_istitle";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "title") {
            if (args.size() != 0) {
                throw SemanticError("str.title() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_title";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "upper") {
            if (args.size() != 0) {
                throw SemanticError("str.upper() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_upper";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "join") {
            if (args.size() != 1) {
                throw SemanticError("str.join() takes one argument",
                    loc);
            }
            ASR::expr_t *arg_sub = args[0].m_value;
            ASR::ttype_t *arg_sub_type = ASRUtils::expr_type(arg_sub);
            if(!ASR::is_a<ASR::List_t>(*arg_sub_type)){
                throw SemanticError("str.join() takes type list only",
                     loc);
            }
            fn_call_name = "_lpython_str_join";
            ASR::call_arg_t str_var;
            str_var.loc = loc;
            str_var.m_value = s_var;
            ASR::call_arg_t list_of_str;
            list_of_str.loc = loc;
            list_of_str.m_value = args[0].m_value;
            fn_args.push_back(al, str_var);
            fn_args.push_back(al, list_of_str);
        } else if (attr_name == "find") {
            if (args.size() != 1) {
                throw SemanticError("str.find() takes one argument",
                    loc);
            }
            ASR::expr_t *arg_sub = args[0].m_value;
            ASR::ttype_t *arg_sub_type = ASRUtils::expr_type(arg_sub);
            if (!ASRUtils::is_character(*arg_sub_type)) {
                throw SemanticError("str.find() takes one argument of type: str",
                    loc);
            }
            fn_call_name = "_lpython_str_find";
            ASR::call_arg_t str;
            str.loc = loc;
            str.m_value = s_var;
            ASR::call_arg_t sub;
            sub.loc = loc;
            sub.m_value = args[0].m_value;
            fn_args.push_back(al, str);
            fn_args.push_back(al, sub);
        } else if (attr_name == "rstrip") {
            if (args.size() != 0) {
                throw SemanticError("str.rstrip() takes no arguments",
                        loc);
            }
            fn_call_name = "_lpython_str_rstrip";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "lstrip") {
            if (args.size() != 0) {
                throw SemanticError("str.lstrip() takes no arguments",
                        loc);
            }
            fn_call_name = "_lpython_str_lstrip";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "strip") {
            if (args.size() != 0) {
                throw SemanticError("str.strip() takes no arguments",
                    loc);
            }
            fn_call_name = "_lpython_str_strip";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "swapcase") {
            if (args.size() != 0) {
                throw SemanticError("str.swapcase() takes no arguments",
                        loc);
            }
            fn_call_name = "_lpython_str_swapcase";
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else if (attr_name == "startswith") {
            if(args.size() != 1) {
                throw SemanticError("str.startswith() takes one argument",
                        loc);
            }
            ASR::expr_t *arg_sub = args[0].m_value;
            ASR::ttype_t *arg_sub_type = ASRUtils::expr_type(arg_sub);
            if (!ASRUtils::is_character(*arg_sub_type)) {
                throw SemanticError("str.startswith() takes one argument of type: str",
                        loc);
            }
            fn_call_name = "_lpython_str_startswith";
            ASR::call_arg_t str;
            str.loc = loc;
            str.m_value = s_var;
            ASR::call_arg_t sub;
            sub.loc = loc;
            sub.m_value = args[0].m_value;
            fn_args.push_back(al, str);
            fn_args.push_back(al, sub);
        } else if (attr_name == "endswith") {
            /*
                str.endswith(suffix)     ---->
                Return True if the string ends with the specified suffix, otherwise return False.

                arg_sub: Substring argument provided inside endswith() function
                arg_sub_type: Type of Substring argument
                fn_call_name: Name of the Function that has logic/implementation of endswith() function
                str: Associates with string on which endswith() function will act on
                suffix: Associates with the suffix string which is provided as an argument to endswith() function
            */
            if(args.size() != 1) {
                throw SemanticError("str.endswith() takes only one argument", loc);
            }
            ASR::expr_t *arg_suffix = args[0].m_value;
            ASR::ttype_t *arg_suffix_type = ASRUtils::expr_type(arg_suffix);
            if (!ASRUtils::is_character(*arg_suffix_type)) {
                throw SemanticError("str.endswith() takes one argument of type: str", loc);
            }

            fn_call_name = "_lpython_str_endswith";
            ASR::call_arg_t str;
            str.loc = loc;
            str.m_value = s_var;

            ASR::call_arg_t suffix;
            suffix.loc = loc;
            suffix.m_value = args[0].m_value;

            // Push string and substring argument on top of Vector (or Function Arguments Stack basically)
            fn_args.push_back(al, str);
            fn_args.push_back(al, suffix);
        } else if (attr_name == "partition") {
            /*
                str.partition(seperator)        ---->

                Split the string at the first occurrence of sep, and return a 3-tuple containing the part
                before the separator, the separator itself, and the part after the separator.
                If the separator is not found, return a 3-tuple containing the string itself, followed
                by two empty strings.
            */
            Vec<ASR::expr_t*> args_; args_.reserve(al, args.n);
            ASRUtils::visit_expr_list(al, args, args_);
            tmp = ASRUtils::Partition::create_partition(al, loc, args_, s_var,
                [&](const std::string &msg, const Location &loc) {
                throw SemanticError(msg, loc); });
            return;
        } else if(attr_name == "count") {
            if(args.size() != 1) {
                throw SemanticError("str.count() takes one argument for now.", loc);
            }
            ASR::expr_t *arg_value = args[0].m_value;
            ASR::ttype_t *arg_value_type = ASRUtils::expr_type(arg_value);
            if (!ASRUtils::is_character(*arg_value_type)) {
                throw SemanticError("str.count() takes one argument of type: str", loc);
            }

            fn_call_name = "_lpython_str_count";
            ASR::call_arg_t str;
            str.loc = loc;
            str.m_value = s_var;

            ASR::call_arg_t value;
            value.loc = loc;
            value.m_value = args[0].m_value;

            // Push string and substring argument on top of Vector (or Function Arguments Stack basically)
            fn_args.push_back(al, str);
            fn_args.push_back(al, value);
        } else if(attr_name == "split") {
            if(args.size() > 1) {
                throw SemanticError("str.split() takes at most one argument for now.", loc);
            }
            fn_call_name = "_lpython_str_split";
            ASR::call_arg_t str;
            str.loc = loc;
            str.m_value = s_var;

            if (args.size() == 1) {
                ASR::expr_t *arg_value = args[0].m_value;
                ASR::ttype_t *arg_value_type = ASRUtils::expr_type(arg_value);
                if (!ASRUtils::is_character(*arg_value_type)) {
                    throw SemanticError("str.split() takes one argument of type: str", loc);
                }
                ASR::call_arg_t value;
                value.loc = loc;
                value.m_value = args[0].m_value;
                fn_args.push_back(al, str);
                fn_args.push_back(al, value);
            } else {
                fn_args.push_back(al, str);
            }
        } else if(attr_name.size() > 2 && attr_name[0] == 'i' && attr_name[1] == 's') {
            /*
                String Validation Methods i.e all "is" based functions are handled here
            */
            std::vector<std::string> validation_methods{"lower", "upper", "decimal", "ascii", "space"};  // Database of validation methods supported
            std::string method_name = attr_name.substr(2);

            if(std::find(validation_methods.begin(),validation_methods.end(), method_name) == validation_methods.end()) {
                throw SemanticError("String method not implemented: " + attr_name, loc);
            }
            if (args.size() != 0) {
                throw SemanticError("str." + attr_name + "() takes no arguments", loc);
            }
            fn_call_name = "_lpython_str_" + attr_name;
            ASR::call_arg_t arg;
            arg.loc = loc;
            arg.m_value = s_var;
            fn_args.push_back(al, arg);
        } else {
            throw SemanticError("String method not implemented: " + attr_name,
                    loc);
        }
        ASR::symbol_t *fn_call = resolve_intrinsic_function(loc, fn_call_name);
        tmp = make_call_helper(al, fn_call, current_scope, fn_args, fn_call_name, loc);
    }

    void handle_constant_string_attributes(std::string &s_var,
                Vec<ASR::call_arg_t> &args, std::string attr_name, const Location &loc) {
        if (attr_name == "capitalize") {
            if (args.size() != 0) {
                throw SemanticError("str.capitalize() takes no arguments",
                    loc);
            }
            for (auto &i : s_var) {
                if (i >= 'A' && i<= 'Z') {
                    i = tolower(i);
                }
            }
            if (s_var.length() > 0) {
                s_var[0] = toupper(s_var[0]);
            }
        } else if (attr_name == "lower") {
            if (args.size() != 0) {
                throw SemanticError("str.lower() takes no arguments",
                        loc);
            }
            for (auto &i : s_var) {
                if (i >= 'A' && i<= 'Z') {
                    i = tolower(i);
                }
            }
        } else if (attr_name == "upper") {
            if (args.size() != 0) {
                throw SemanticError("str.upper() takes no arguments",
                        loc);
            }
            for (auto &i : s_var) {
                if (i >= 'a' && i<= 'z') {
                    i = toupper(i);
                }
            }
        } else if (attr_name == "find") {
            if (args.size() != 1) {
                throw SemanticError("str.find() takes one arguments",
                        loc);
            }
            ASR::expr_t *arg = args[0].m_value;
            ASR::ttype_t *type = ASRUtils::expr_type(arg);
            if (!ASRUtils::is_character(*type)) {
                throw SemanticError("str.find() takes one arguments of type: str",
                    arg->base.loc);
            }
            if (ASRUtils::expr_value(arg) != nullptr) {
                ASR::StringConstant_t* sub_str_con = ASR::down_cast<ASR::StringConstant_t>(arg);
                std::string sub = sub_str_con->m_s;
                int res = ASRUtils::KMP_string_match(s_var, sub);
                tmp = ASR::make_IntegerConstant_t(al, loc, res,
                    ASRUtils::TYPE(ASR::make_Integer_t(al,loc, 4)));
            } else {
                ASR::symbol_t *fn_div = resolve_intrinsic_function(loc, "_lpython_str_find");
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 1);
                ASR::call_arg_t str_arg;
                str_arg.loc = loc;
                ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        1, s_var.size(), nullptr));
                str_arg.m_value = ASRUtils::EXPR(
                        ASR::make_StringConstant_t(al, loc, s2c(al, s_var), str_type));
                ASR::call_arg_t sub_arg;
                sub_arg.loc = loc;
                sub_arg.m_value = arg;
                args.push_back(al, str_arg);
                args.push_back(al, sub_arg);
                tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_find", loc);
            }
            return;
        } else if (attr_name == "rstrip") {
            if (args.size() != 0) {
                throw SemanticError("str.rstrip() takes no arguments",
                    loc);
            }
            int ind = (int)s_var.size() - 1;
            while (ind >= 0 && s_var[ind] == ' '){
                ind--;
            }
            s_var = std::string(s_var.begin(), s_var.begin() + ind + 1);
        } else if (attr_name == "lstrip") {
            if (args.size() != 0) {
                throw SemanticError("str.lstrip() takes no arguments",
                        loc);
            }
            size_t ind = 0;
            while (ind < s_var.size() && s_var[ind] == ' ') {
                ind++;
            }
            s_var = std::string(s_var.begin() + ind, s_var.end());
        } else if (attr_name == "strip") {
            if (args.size() != 0) {
                throw SemanticError("str.strip() takes no arguments",
                        loc);
            }
            size_t l = 0;
            int r = (int)s_var.size() - 1;
            while (l < s_var.size() && (int)r >= 0 && (s_var[l] == ' ' || s_var[r] == ' ')) {
                l += s_var[l] == ' ';
                r -= s_var[r] == ' ';
            }
            s_var = std::string(s_var.begin() + l, s_var.begin() + r + 1);
        } else if (attr_name == "swapcase") {
            if (args.size() != 0) {
                throw SemanticError("str.swapcase() takes no arguments",
                        loc);
            }
            for (size_t i = 0; i < s_var.size(); i++)  {
                char &cur = s_var[i];
                if(cur >= 'a' && cur <= 'z') {
                    cur = cur -'a' + 'A';
                } else if(cur >= 'A' && cur <= 'Z') {
                    cur = cur - 'A' + 'a';
                }
            }
        } else if (attr_name == "startswith") {
            if (args.size() != 1) {
                throw SemanticError("str.startswith() takes one arguments",
                        loc);
            }
            ASR::expr_t *arg = args[0].m_value;
            ASR::ttype_t *type = ASRUtils::expr_type(arg);
            if (!ASRUtils::is_character(*type)) {
                throw SemanticError("str.startwith() takes one arguments of type: str",
                    arg->base.loc);
            }
            if (ASRUtils::expr_value(arg) != nullptr) {
                ASR::StringConstant_t* sub_str_con = ASR::down_cast<ASR::StringConstant_t>(arg);
                std::string sub = sub_str_con->m_s;
                size_t ind1 = 0, ind2 = 0;
                bool res = !(s_var.size() == 0 && sub.size());
                while ((ind1 < s_var.size()) && (ind2 < sub.size()) && res) {
                    res &= s_var[ind1] == sub[ind2];
                    ind1++;
                    ind2++;
                }
                res &= res ? (ind2 == sub.size()): true;
                tmp = ASR::make_LogicalConstant_t(al, loc, res,
                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
            } else {
                ASR::symbol_t *fn_div = resolve_intrinsic_function(loc, "_lpython_str_startswith");
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 1);
                ASR::call_arg_t str_arg;
                str_arg.loc = loc;
                ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        1, s_var.size(), nullptr));
                str_arg.m_value = ASRUtils::EXPR(
                        ASR::make_StringConstant_t(al, loc, s2c(al, s_var), str_type));
                ASR::call_arg_t sub_arg;
                sub_arg.loc = loc;
                sub_arg.m_value = arg;
                args.push_back(al, str_arg);
                args.push_back(al, sub_arg);
                tmp = make_call_helper(al, fn_div, current_scope, args,
                        "_lpython_str_startswith", loc);
            }
            return;
        } else if (attr_name == "endswith") {
            /*
                str.endswith(suffix)     ---->
                Return True if the string ends with the specified suffix, otherwise return False.
            */

            if (args.size() != 1) {
                throw SemanticError("str.endswith() takes one arguments", loc);
            }

            ASR::expr_t *arg_suffix = args[0].m_value;
            ASR::ttype_t *arg_suffix_type = ASRUtils::expr_type(arg_suffix);
            if (!ASRUtils::is_character(*arg_suffix_type)) {
                throw SemanticError("str.endswith() takes one arguments of type: str", arg_suffix->base.loc);
            }

            if (ASRUtils::expr_value(arg_suffix) != nullptr) {
                /*
                    Invoked when Suffix argument is provided as a constant string
                */
                ASR::StringConstant_t* suffix_constant = ASR::down_cast<ASR::StringConstant_t>(arg_suffix);
                std::string suffix = suffix_constant->m_s;

                bool res = true;
                if (suffix.size() > s_var.size())
                    res = false;
                else
                    res = std::equal(suffix.rbegin(), suffix.rend(), s_var.rbegin());

                tmp = ASR::make_LogicalConstant_t(al, loc, res,
                    ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));

            } else {
                /*
                    Invoked when Suffix argument is provided as a variable
                    b: str = "ple"
                    Eg: "apple".endswith(b)
                */
                ASR::symbol_t *fn_div = resolve_intrinsic_function(loc, "_lpython_str_endswith");
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 1);
                ASR::call_arg_t str_arg;
                str_arg.loc = loc;
                ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                        1, s_var.size(), nullptr));
                str_arg.m_value = ASRUtils::EXPR(
                        ASR::make_StringConstant_t(al, loc, s2c(al, s_var), str_type));
                ASR::call_arg_t sub_arg;
                sub_arg.loc = loc;
                sub_arg.m_value = arg_suffix;
                args.push_back(al, str_arg);
                args.push_back(al, sub_arg);

                tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_endswith", loc);
            }
            return;
        } else if (attr_name == "partition") {
            /*
                str.partition(seperator)        ---->
                Split the string at the first occurrence of sep, and return a 3-tuple containing the part
                before the separator, the separator itself, and the part after the separator.
                If the separator is not found, return a 3-tuple containing the string itself, followed
                by two empty strings.
            */
            Vec<ASR::expr_t*> args_; args_.reserve(al, args.n);
            ASRUtils::visit_expr_list(al, args, args_);
            if(s_var.size() == 0) {
                throw SemanticError("String to undergo partition cannot be empty",
                    loc);
            }
            ASR::ttype_t *char_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                loc, 1, s_var.size(), nullptr));
            ASR::expr_t *str = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
                loc, s2c(al, s_var), char_type));
            tmp = ASRUtils::Partition::create_partition(al, loc, args_, str,
                [&](const std::string &msg, const Location &loc) {
                throw SemanticError(msg, loc); });
            return;
        } else if (attr_name.size() > 2 && attr_name[0] == 'i' && attr_name[1] == 's') {
            /*
                * Specification -

                Return True if all cased characters [lowercase, uppercase, titlecase] in the string
                are lowercase and there is at least one cased character, False otherwise.

                * islower() method is limited to English Alphabets currently
                * TODO: We can support other characters from Unicode Library
            */
            std::vector<std::string> validation_methods{"lower", "upper", "decimal", "ascii", "space"};  // Database of validation methods supported
            std::string method_name = attr_name.substr(2);
            if(std::find(validation_methods.begin(),validation_methods.end(), method_name) == validation_methods.end()) {
                throw SemanticError("String method not implemented: " + attr_name, loc);
            }
            if (args.size() != 0) {
                throw SemanticError("str." + attr_name + "() takes no arguments", loc);
            }

            if(attr_name == "islower") {
                /*
                    * Specification:
                    Return True if all cased characters in the string are lowercase and there is at least one cased character, False otherwise.
                */
                bool is_cased_present = false;
                bool is_lower = true;
                for (auto &i : s_var) {
                    if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
                        is_cased_present = true;
                        if(!(i >= 'a' && i <= 'z')) {
                            is_lower = false;
                            break;
                        }
                    }
                }
                is_lower = is_lower && is_cased_present;
                tmp = ASR::make_LogicalConstant_t(al, loc, is_lower,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
                return;
            } else if(attr_name == "isupper") {
                /*
                    * Specification:
                    Return True if all cased characters in the string are uppercase and there is at least one cased character, False otherwise.
                */
                bool is_cased_present = false;
                bool is_upper = true;
                for (auto &i : s_var) {
                    if ((i >= 'A' && i <= 'Z') || (i >= 'a' && i <= 'z')) {
                        is_cased_present = true;
                        if(!(i >= 'A' && i <= 'Z')) {
                            is_upper = false;
                            break;
                        }
                    }
                }
                is_upper = is_upper && is_cased_present;
                tmp = ASR::make_LogicalConstant_t(al, loc, is_upper,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
                return;
            } else if(attr_name == "isdecimal") {
                /*
                    * Specification:
                    Return True if all characters in the string are decimal characters and there is at least one character, False otherwise.
                */
                bool is_decimal = (s_var.size() != 0);
                for(auto &i: s_var) {
                    if(i < '0' || i > '9') {
                        is_decimal = false;
                        break;
                    }
                }
                tmp = ASR::make_LogicalConstant_t(al, loc, is_decimal,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
                return;
            } else if(attr_name == "isascii") {
                /*
                    * Specification -
                    Return True if the string is empty or all characters in the string are ASCII, False otherwise.
                    ASCII characters have code points in the range U+0000-U+007F.
                */
                bool is_ascii = true;
                for(char i: s_var) {
                    if (static_cast<unsigned int>(i) > 127) {
                        is_ascii = false;
                        break;
                    }
                }
                tmp = ASR::make_LogicalConstant_t(al, loc, is_ascii,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
                return;
            } else if (attr_name == "isspace") {
                /*
                  * Specification:
Return true if all characters in the input string are considered whitespace
characters, as defined by CPython. Return false otherwise. For now we use the
std::isspace function, but if we later discover that it differs from CPython,
we will have to use something else.
                */
                bool is_space = (s_var.size() != 0);
                for (char i : s_var) {
                    if (!std::isspace(static_cast<unsigned char>(i))) {
                        is_space = false;
                        break;
                    }
                }
                tmp = ASR::make_LogicalConstant_t(al, loc, is_space,
                        ASRUtils::TYPE(ASR::make_Logical_t(al, loc, 4)));
                return;
            } else {
                throw SemanticError("'str' object has no attribute '" + attr_name + "'", loc);
            }
        } else {
            throw SemanticError("'str' object has no attribute '" + attr_name + "'",
                    loc);
        }
        ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, s_var.size(), nullptr));
        tmp = ASR::make_StringConstant_t(al, loc, s2c(al, s_var), str_type);
    }

    void handle_attribute(AST::Attribute_t* at, Vec<ASR::call_arg_t> &args, const Location &loc) {
        if (AST::is_a<AST::Name_t>(*at->m_value)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(at->m_value);
            std::string mod_name = n->m_id;
            std::string call_name = at->m_attr;
            std::string call_name_store = "__" + mod_name + "_" + call_name;
            ASR::symbol_t *st = nullptr;
            if (current_scope->resolve_symbol(call_name_store) != nullptr) {
                st = current_scope->get_symbol(call_name_store);
            } else {
                st = current_scope->resolve_symbol(mod_name);
                std::set<std::string> symbolic_attributes = {
                    "diff", "expand", "has"
                };
                std::set<std::string> symbolic_constants = {
                    "pi", "E"
                };
                if (symbolic_attributes.find(call_name) != symbolic_attributes.end() &&
                    symbolic_constants.find(mod_name) != symbolic_constants.end()){
                        ASRUtils::create_intrinsic_function create_func;
                        create_func = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function(mod_name);
                        Vec<ASR::expr_t*> eles; eles.reserve(al, args.size());
                        Vec<ASR::expr_t*> args_; args_.reserve(al, 1);
                        for (size_t i=0; i<args.size(); i++) {
                            eles.push_back(al, args[i].m_value);
                        }
                        tmp = create_func(al, at->base.base.loc, args_,
                            [&](const std::string &msg, const Location &loc) {
                            throw SemanticError(msg, loc); });
                        handle_symbolic_attribute(ASRUtils::EXPR(tmp), call_name, loc, eles);
                        return;
                }
                if (!st) {
                    throw SemanticError("NameError: '" + mod_name + "' is not defined", n->base.base.loc);
                }
                if( ASR::is_a<ASR::Module_t>(*st) ) {
                    ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(st);
                    call_name_store = ASRUtils::get_mangled_name(m, call_name_store);
                    st = import_from_module(al, m, current_scope, mod_name,
                                        call_name, call_name_store, loc);
                    current_scope->add_symbol(call_name_store, st);
                } else if( ASR::is_a<ASR::StructType_t>(*st) ) {
                    st = get_struct_member(st, call_name, loc);
                } else if ( ASR::is_a<ASR::Variable_t>(*st)) {
                    ASR::Variable_t* var = ASR::down_cast<ASR::Variable_t>(st);
                    if (ASR::is_a<ASR::Struct_t>(*var->m_type)) {
                        // call to struct member function
                        ASR::Struct_t* var_struct = ASR::down_cast<ASR::Struct_t>(var->m_type);
                        st = get_struct_member(var_struct->m_derived_type, call_name, loc);
                    } else {
                        // this case when we have variable and attribute
                        st = current_scope->resolve_symbol(mod_name);
                        Vec<ASR::expr_t*> eles;
                        eles.reserve(al, args.size());
                        for (size_t i=0; i<args.size(); i++) {
                            eles.push_back(al, args[i].m_value);
                        }
                        ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(
                                        ASR::make_Var_t(al, loc, st));
                        if (ASR::is_a<ASR::Character_t>(*(ASRUtils::expr_type(se)))) {
                            handle_string_attributes(se, args, at->m_attr, loc);
                            return;
                        }
                        ASR::ttype_t *type = ASRUtils::expr_type(se);
                        if (ASR::is_a<ASR::SymbolicExpression_t>(*type)) {
                            handle_symbolic_attribute(se, at->m_attr, loc, eles);
                            return;
                        }
                        handle_builtin_attribute(se, at->m_attr, loc, eles);
                        return;
                    }
                }
            }
            tmp = make_call_helper(al, st, current_scope, args, call_name, loc);
            return;
        } else if (AST::is_a<AST::UnaryOp_t>(*at->m_value)) {
            AST::UnaryOp_t* uop = AST::down_cast<AST::UnaryOp_t>(at->m_value);
            visit_UnaryOp(*uop);
            Vec<ASR::expr_t*> eles;
            eles.reserve(al, args.size());
            for (size_t i=0; i<args.size(); i++) {
                eles.push_back(al, args[i].m_value);
            }
            ASR::expr_t* expr = ASR::down_cast<ASR::expr_t>(tmp);
            handle_builtin_attribute(expr, at->m_attr, loc, eles);
            return;
        } else if (AST::is_a<AST::BinOp_t>(*at->m_value)) {
            AST::BinOp_t* bop = AST::down_cast<AST::BinOp_t>(at->m_value);
            std::set<std::string> symbolic_attributes = {
                "diff", "expand", "has"
            };
            if (symbolic_attributes.find(at->m_attr) != symbolic_attributes.end()){
                switch (bop->m_op) {
                    case (AST::operatorType::Add) :
                    case (AST::operatorType::Sub) :
                    case (AST::operatorType::Mult) :
                    case (AST::operatorType::Div) :
                    case (AST::operatorType::Pow) : {
                        visit_BinOp(*bop);
                        Vec<ASR::expr_t*> eles;
                        eles.reserve(al, args.size());
                        for (size_t i=0; i<args.size(); i++) {
                            eles.push_back(al, args[i].m_value);
                        }
                        handle_symbolic_attribute(ASRUtils::EXPR(tmp), at->m_attr, loc, eles);
                        return;
                    }
                    default : {
                        throw SemanticError("Binary operator type not supported", loc);
                    }
                }
            }
        } else if (AST::is_a<AST::ConstantInt_t>(*at->m_value)) {
            if (std::string(at->m_attr) == std::string("bit_length")) {
                //bit_length() attribute:
                if(args.size() != 0) {
                    throw SemanticError("int.bit_length() takes no arguments", loc);
                }
                AST::ConstantInt_t *n = AST::down_cast<AST::ConstantInt_t>(at->m_value);
                int64_t int_val = std::abs(n->m_value);
                int32_t res = 0;
                for(; int_val; int_val >>= 1, res++);
                ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                tmp = ASR::make_IntegerConstant_t(al, loc, res, int_type);
                return;
            } else {
                throw SemanticError("'int' object has no attribute '" + std::string(at->m_attr) + "'", loc);
            }
        } else if (AST::is_a<AST::ConstantStr_t>(*at->m_value)) {
            AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
            std::string res = n->m_value;
            handle_constant_string_attributes(res, args, at->m_attr, loc);
            return;
        } else if (AST::is_a<AST::Call_t>(*at->m_value)) {
            AST::Call_t* call = AST::down_cast<AST::Call_t>(at->m_value);
            std::set<std::string> symbolic_attributes = {
                "diff", "expand", "has"
            };
            if (symbolic_attributes.find(at->m_attr) != symbolic_attributes.end()){
                std::set<std::string> symbolic_functions = {
                    "sin", "cos", "log", "exp", "Abs", "Symbol"
                };
                if (AST::is_a<AST::Attribute_t>(*call->m_func)) {
                    visit_Call(*call);
                    Vec<ASR::expr_t*> eles;
                    eles.reserve(al, args.size());
                    for (size_t i=0; i<args.size(); i++) {
                        eles.push_back(al, args[i].m_value);
                    }
                    handle_symbolic_attribute(ASRUtils::EXPR(tmp), at->m_attr, loc, eles);
                    return;
                } else if (AST::is_a<AST::Name_t>(*call->m_func)) {
                    AST::Name_t *n = AST::down_cast<AST::Name_t>(call->m_func);
                    std::string call_name = n->m_id;
                    if (symbolic_functions.find(call_name) != symbolic_functions.end()) {
                        visit_Call(*call);
                        Vec<ASR::expr_t*> eles;
                        eles.reserve(al, args.size());
                        for (size_t i=0; i<args.size(); i++) {
                            eles.push_back(al, args[i].m_value);
                        }
                        handle_symbolic_attribute(ASRUtils::EXPR(tmp), at->m_attr, loc, eles);
                        return;
                    } else {
                        throw SemanticError(std::string(call_name) + " not supported in Call", loc);
                    }
                }
            }
        } else {
            throw SemanticError("Only Name type and constant integers supported in Call", loc);
        }
    }

    ASR::symbol_t* get_struct_member(ASR::symbol_t* struct_type_sym, std::string &call_name, const Location &loc) {
        ASR::StructType_t* struct_type = ASR::down_cast<ASR::StructType_t>(struct_type_sym);
        std::string struct_var_name = struct_type->m_name;
        std::string struct_member_name = call_name;
        ASR::symbol_t* struct_member = struct_type->m_symtab->resolve_symbol(struct_member_name);
        ASR::symbol_t* struct_mem_asr_owner = ASRUtils::get_asr_owner(struct_member);
        if( !struct_member || !struct_mem_asr_owner ||
            !ASR::is_a<ASR::StructType_t>(*struct_mem_asr_owner) ) {
            throw SemanticError(struct_member_name + " not present in " +
                                struct_var_name + " dataclass", loc);
        }
        std::string import_name = struct_var_name + "_" + struct_member_name;
        ASR::symbol_t* import_struct_member = current_scope->resolve_symbol(import_name);
        bool import_from_struct = true;
        if( import_struct_member ) {
            if( ASR::is_a<ASR::ExternalSymbol_t>(*import_struct_member) ) {
                ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(import_struct_member);
                if( ext_sym->m_external == struct_member &&
                    std::string(ext_sym->m_module_name) == struct_var_name ) {
                    import_from_struct = false;
                }
            }
        }
        if( import_from_struct ) {
            import_name = current_scope->get_unique_name(import_name, false);
            import_struct_member = ASR::down_cast<ASR::symbol_t>(ASR::make_ExternalSymbol_t(al,
                                        loc, current_scope, s2c(al, import_name),
                                        struct_member, s2c(al, struct_var_name), nullptr, 0,
                                        s2c(al, struct_member_name), ASR::accessType::Public));
            current_scope->add_symbol(import_name, import_struct_member);
        }
        return import_struct_member;
    }

    void parse_args(const AST::Call_t &x, Vec<ASR::call_arg_t> &args) {
        // Keyword arguments handled in make_call_helper()
        if( x.n_keywords == 0 ) {
            args.reserve(al, x.n_args);
            visit_expr_list(x.m_args, x.n_args, args);
        }
    }

    void visit_Call(const AST::Call_t &x) {
        std::string call_name = "";
        Vec<ASR::call_arg_t> args;
        if (AST::is_a<AST::Name_t>(*x.m_func)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(x.m_func);
            call_name = n->m_id;
        } else if (AST::is_a<AST::Attribute_t>(*x.m_func)) {
            parse_args(x, args);
            AST::Attribute_t *at = AST::down_cast<AST::Attribute_t>(x.m_func);
            handle_attribute(at, args, x.base.base.loc);
            return;
        } else {
            throw SemanticError("Only Name or Attribute type supported in Call",
                x.base.base.loc);
        }

        ASR::symbol_t *s = current_scope->resolve_symbol(call_name);
        if( s && ASR::is_a<ASR::Module_t>(*s) ) {
            std::string mangled_name = ASRUtils::get_mangled_name(ASR::down_cast<ASR::Module_t>(s), call_name);
            s = current_scope->resolve_symbol(mangled_name);
        }

        if (!s) {
            std::string intrinsic_name = call_name;
            std::set<std::string> not_cpython_builtin = {
                "sin", "cos", "gamma", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "exp", "exp2", "expm1", "Symbol", "diff", "expand", "trunc", "fix",
                "sum" // For sum called over lists
            };
            std::set<std::string> symbolic_functions = {
                "sin", "cos", "log", "exp", "Abs"
            };
            if ((symbolic_functions.find(call_name) != symbolic_functions.end()) &&
                imported_functions[call_name] == "sympy"){
                intrinsic_name = "Symbolic" + std::string(1, std::toupper(call_name[0])) + call_name.substr(1);
            }
            if ((ASRUtils::IntrinsicScalarFunctionRegistry::is_intrinsic_function(intrinsic_name) ||
                ASRUtils::IntrinsicArrayFunctionRegistry::is_intrinsic_function(intrinsic_name)) &&
                (not_cpython_builtin.find(call_name) == not_cpython_builtin.end() ||
                imported_functions.find(call_name) != imported_functions.end() )) {
                ASRUtils::create_intrinsic_function create_func;
                if (ASRUtils::IntrinsicScalarFunctionRegistry::is_intrinsic_function(intrinsic_name)) {
                    create_func = ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function(intrinsic_name);
                } else {
                    create_func = ASRUtils::IntrinsicArrayFunctionRegistry::get_create_function(intrinsic_name);
                }
                Vec<ASR::expr_t*> args_; args_.reserve(al, x.n_args);
                visit_expr_list(x.m_args, x.n_args, args_);
                if (ASRUtils::is_array(ASRUtils::expr_type(args_[0])) &&
                    imported_functions[call_name] == "math" ) {
                    throw SemanticError("Function '" + call_name + "' does not accept vector values",
                        x.base.base.loc);
                }
                tmp = create_func(al, x.base.base.loc, args_,
                    [&](const std::string &msg, const Location &loc) {
                    throw SemanticError(msg, loc); });
                return ;
            } else if (intrinsic_procedures.is_intrinsic(call_name)) {
                s = resolve_intrinsic_function(x.base.base.loc, call_name);
                if (call_name == "pow") {
                    diag.add(diag::Diagnostic(
                        "Could have used '**' instead of 'pow'",
                        diag::Level::Style, diag::Stage::Semantic, {
                            diag::Label("'**' could be used instead",
                                    {x.base.base.loc}),
                        })
                    );
                }
            } else {
                // TODO: We need to port all functions below to the intrinsic functions file
                // Then we can uncomment this error message:
                /*
                throw SemanticError("The function '" + call_name + "' is not declared and not intrinsic",
                    x.base.base.loc);
                */
            if (call_name == "print") {
                args.reserve(al, x.n_args);
                visit_expr_list(x.m_args, x.n_args, args);
                Vec<ASR::expr_t*> args_expr = ASRUtils::call_arg2expr(al, args);
                ASR::expr_t *separator = nullptr;
                ASR::expr_t *end = nullptr;
                if (x.n_keywords > 0) {
                    std::string arg_name;
                    for (size_t i = 0; i < x.n_keywords; i++) {
                        arg_name = x.m_keywords[i].m_arg;
                        if (arg_name == "sep") {
                            visit_expr(*x.m_keywords[i].m_value);
                            separator = ASRUtils::EXPR(tmp);
                            ASR::ttype_t *type = ASRUtils::expr_type(separator);
                            if (!ASRUtils::is_character(*type)) {
                                std::string found = ASRUtils::type_to_str(type);
                                diag.add(diag::Diagnostic(
                                    "Separator is expected to be of string type",
                                    diag::Level::Error, diag::Stage::Semantic, {
                                        diag::Label("Expected string, found: " + found,
                                                {separator->base.loc})
                                    })
                                );
                                throw SemanticAbort();
                            }
                        }
                        if (arg_name == "end") {
                            visit_expr(*x.m_keywords[i].m_value);
                            end = ASRUtils::EXPR(tmp);
                            ASR::ttype_t *type = ASRUtils::expr_type(end);
                            if (!ASRUtils::is_character(*type)) {
                                std::string found = ASRUtils::type_to_str(type);
                                diag.add(diag::Diagnostic(
                                    "End is expected to be of string type",
                                    diag::Level::Error, diag::Stage::Semantic, {
                                        diag::Label("Expected string, found: " + found,
                                                {end->base.loc})
                                    })
                                );
                                throw SemanticAbort();
                            }
                        }
                    }
                }
                tmp = ASR::make_Print_t(al, x.base.base.loc,
                    args_expr.p, args_expr.size(), separator, end);
                return;
            } else if (call_name == "quit") {
                parse_args(x, args);
                ASR::expr_t *code;
                if (args.size() == 0) {
                    code = nullptr;
                } else if (args.size() == 1) {
                    code = args[0].m_value;
                } else {
                    throw SemanticError("The function quit() requires 0 or 1 arguments",
                        x.base.base.loc);
                }
                tmp = ASR::make_Stop_t(al, x.base.base.loc, code);
                return;
            } else if( call_name == "reserve" ) {
                parse_args(x, args);
                ASRUtils::create_intrinsic_function create_func =
                    ASRUtils::IntrinsicScalarFunctionRegistry::get_create_function("reserve");
                Vec<ASR::expr_t*> args_exprs = ASRUtils::call_arg2expr(al, args);
                tmp = create_func(al, x.base.base.loc, args_exprs,
                    [&](const std::string &msg, const Location &loc) {
                    throw SemanticError(msg, loc); });
                return ;
            } else if (call_name == "size") {
                parse_args(x, args);
                if( args.size() < 1 || args.size() > 2 ) {
                    throw SemanticError("array accepts only 1 (arr) or 2 (arr, axis) arguments, got " +
                                        std::to_string(args.size()) + " arguments instead.",
                                        x.base.base.loc);
                }
                const Location &loc = x.base.base.loc;
                ASR::expr_t *var = args[0].m_value;
                ASR::expr_t *dim = nullptr;
                ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc, 4));
                if (args.size() == 2) {
                    ASR::expr_t* const_one = ASRUtils::EXPR(make_IntegerConstant_t(al, loc, 1, int_type));
                    dim = ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc,
                        args[1].m_value, ASR::binopType::Add, const_one, int_type, nullptr));
                }
                tmp = ASRUtils::make_ArraySize_t_util(al, loc, var, dim, int_type, nullptr, false);
                return;
            } else if (call_name == "empty") {
                if (x.n_args != 1 || x.n_keywords != 1) {
                    throw SemanticError("empty() expects 1 positional argument for shape"
                        " and 1 keyword argument for 'dtype'",
                        x.base.base.loc);
                }

                ASR::ttype_t* type = nullptr;
                bool is_allocatable = ASRUtils::is_allocatable(assign_asr_target);
                Vec<ASR::dimension_t> dims;
                dims.reserve(al, 0);

                visit_expr(*x.m_args[0]);
                ASR::expr_t* shape = ASRUtils::EXPR(tmp);
                fill_dims_for_asr_type(dims, shape, shape->base.loc, is_allocatable);

                std::string keyword_arg = x.m_keywords[0].m_arg;
                if (keyword_arg != "dtype") {
                    throw SemanticError("Unexpected keyword argument '" + keyword_arg + "', expected 'dtype'",
                                x.m_keywords[0].loc);
                }

                std::string dtype_np = "";
                if( AST::is_a<AST::Name_t>(*x.m_keywords[0].m_value) ) {
                    AST::Name_t* name_t = AST::down_cast<AST::Name_t>(x.m_keywords[0].m_value);
                    dtype_np = name_t->m_id;
                } else {
                    LCOMPILERS_ASSERT(false);
                }

                if (numpy2lpythontypes.find(dtype_np) != numpy2lpythontypes.end()) {
                    dtype_np = numpy2lpythontypes[dtype_np];
                }

                type = get_type_from_var_annotation(dtype_np, x.m_keywords[0].m_value->base.loc, dims);
                if (is_allocatable) {
                    const Location& loc = x.base.base.loc;
                    Vec<ASR::alloc_arg_t> alloc_args_vec;
                    alloc_args_vec.reserve(al, 1);
                    ASR::alloc_arg_t new_arg;
                    new_arg.loc = loc;
                    new_arg.m_len_expr = nullptr;
                    new_arg.m_type = nullptr;
                    new_arg.m_dims = dims.p;
                    new_arg.n_dims = dims.size();
                    new_arg.m_a = assign_asr_target;
                    alloc_args_vec.push_back(al, new_arg);
                    tmp = ASR::make_Allocate_t(al, loc,
                                alloc_args_vec.p, alloc_args_vec.size(),
                                nullptr, nullptr, nullptr);
                } else {
                    Vec<ASR::expr_t*> arr_args;
                    arr_args.reserve(al, 0);
                    tmp = ASRUtils::make_ArrayConstant_t_util(al, x.base.base.loc,
                        arr_args.p, arr_args.size(), type, ASR::arraystorageType::RowMajor);
                }
                return;
            } else if (call_name == "c_p_pointer") {
                tmp = create_CPtrToPointer(x);
                return;
            } else if( call_name == "p_c_pointer" && !s ) {
                tmp = create_PointerToCPtr(x);
                return;
            } else if (call_name == "empty_c_void_p") {
                // TODO: check that `empty_c_void_p uses` has arguments that are compatible
                // with the type
                ASR::ttype_t* type;
                if (assign_asr_target) {
                    type = ASRUtils::expr_type(assign_asr_target);
                } else {
                    type = ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc));
                }
                tmp = ASR::make_PointerNullConstant_t(al, x.base.base.loc, type);
                return;
            } else if (call_name == "cptr_to_u64") {
                parse_args(x, args);
                if (args.size() != 1) {
                    throw SemanticError("cptr_to_u64 accpets exactly 1 argument",
                                        x.base.base.loc);
                }

                ASR::expr_t *var = args[0].m_value;
                ASR::ttype_t *a_type = ASRUtils::expr_type(var);
                if (!ASR::is_a<ASR::CPtr_t>(*a_type)) {
                    throw SemanticError("Argument of type `CPtr` is expected in cptr_to_u64",
                                        x.base.base.loc);
                }
                ASR::ttype_t *u_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al,
                                x.base.base.loc, 8));

                tmp = ASR::make_Cast_t(al, x.base.base.loc,
                    var, ASR::cast_kindType::CPtrToUnsignedInteger, u_type, nullptr);
                return;
            } else if (call_name == "u64_to_cptr") {
                parse_args(x, args);
                if (args.size() != 1) {
                    throw SemanticError("u64_to_cptr accpets exactly 1 argument",
                                        x.base.base.loc);
                }

                ASR::expr_t *var = args[0].m_value;
                ASR::ttype_t *a_type = ASRUtils::expr_type(var);
                if (!ASR::is_a<ASR::UnsignedInteger_t>(*a_type)) {
                    throw SemanticError("Argument of type `u64` is expected in u64_to_cptr",
                                        x.base.base.loc);
                }
                ASR::ttype_t *c_type = ASRUtils::TYPE(ASR::make_CPtr_t(al, x.base.base.loc));

                tmp = ASR::make_Cast_t(al, x.base.base.loc,
                    var, ASR::cast_kindType::UnsignedIntegerToCPtr, c_type, nullptr);
                return;
            } else if (call_name == "TypeVar") {
                // Ignore TypeVar for now, we handle it based on the identifier itself
                tmp = nullptr;
                return;
            } else if (call_name == "callable") {
                parse_args(x, args);
                if (args.size() != 1) {
                    throw SemanticError(call_name + "() takes exactly one argument (" +
                        std::to_string(args.size()) + " given)", x.base.base.loc);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4));
                ASR::expr_t *arg = args[0].m_value;
                bool result = false;
                if (ASR::is_a<ASR::Var_t>(*arg)) {
                    ASR::symbol_t *t = ASR::down_cast<ASR::Var_t>(arg)->m_v;
                    result = ASR::is_a<ASR::Function_t>(*t);
                }
                tmp = ASR::make_LogicalConstant_t(al, x.base.base.loc, result, type);
                return;
            } else if( call_name == "pointer" ) {
                parse_args(x, args);
                ASR::ttype_t* type_ = ASRUtils::duplicate_type_with_empty_dims(
                    al, ASRUtils::expr_type(args[0].m_value),
                    ASR::array_physical_typeType::DescriptorArray, true);
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Pointer_t(al, x.base.base.loc, type_));
                tmp = ASR::make_GetPointer_t(al, x.base.base.loc, args[0].m_value, type, nullptr);
                return ;
            } else if( call_name.substr(0, 6) == "bitnot" ) {
                parse_args(x, args);
                if (args.size() != 1) {
                    throw SemanticError(call_name + "() expects one argument, provided " + std::to_string(args.size()), x.base.base.loc);
                }
                ASR::expr_t* operand = args[0].m_value;
                ASR::ttype_t *operand_type = ASRUtils::expr_type(operand);
                ASR::expr_t* value = nullptr;
                if (!ASR::is_a<ASR::UnsignedInteger_t>(*operand_type)) {
                    throw SemanticError(call_name + "() expects unsigned integer, provided" + ASRUtils::type_to_str_python(operand_type), x.base.base.loc);
                }
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::UnsignedIntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    uint64_t val = ~uint64_t(op_value);
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_UnsignedIntegerConstant_t(
                        al, x.base.base.loc, val, operand_type));
                }
                tmp = ASR::make_UnsignedIntegerBitNot_t(al, x.base.base.loc, operand, operand_type, value);
                return;
            } else if( call_name == "array" ) {
                parse_args(x, args);
                ASR::ttype_t* type = nullptr;
                if( x.n_keywords > 0) {
                    args.reserve(al, 1);
                    visit_expr_list(x.m_args, x.n_args, args);
                    if( x.n_keywords > 1 ) {
                        throw SemanticError("More than one keyword "
                            "arguments aren't recognised by array",
                            x.base.base.loc);
                    }
                    if( std::string(x.m_keywords[0].m_arg) != "dtype" ) {
                        throw SemanticError("Unrecognised keyword argument, " +
                            std::string(x.m_keywords[0].m_arg), x.base.base.loc);
                    }
                    std::string dtype_np = "";
                    if( AST::is_a<AST::Name_t>(*x.m_keywords[0].m_value) ) {
                        AST::Name_t* name_t = AST::down_cast<AST::Name_t>(x.m_keywords[0].m_value);
                        dtype_np = name_t->m_id;
                    } else {
                        LCOMPILERS_ASSERT(false);
                    }
                    LCOMPILERS_ASSERT(numpy2lpythontypes.find(dtype_np) != numpy2lpythontypes.end());
                    Vec<ASR::dimension_t> dims;
                    dims.n = 0;
                    type = get_type_from_var_annotation(
                        numpy2lpythontypes[dtype_np], x.base.base.loc, dims);
                }
                if( args.size() != 1 ) {
                    throw SemanticError("array accepts only 1 argument for now, got " +
                                        std::to_string(args.size()) + " arguments instead.",
                                        x.base.base.loc);
                }
                ASR::expr_t *arg = args[0].m_value;
                if( type == nullptr ) {
                    type = ASRUtils::expr_type(arg);
                }
                if(ASR::is_a<ASR::ListConstant_t>(*arg)) {
                    type = ASRUtils::get_contained_type(type);
                    ASR::ListConstant_t* list = ASR::down_cast<ASR::ListConstant_t>(arg);
                    ASR::expr_t **m_args = list->m_args;
                    size_t n_args = list->n_args;
                    Vec<ASR::dimension_t> dims;
                    dims.reserve(al, 1);
                    ASR::dimension_t dim;
                    dim.loc = x.base.base.loc;
                    dim.m_length = make_ConstantWithKind(make_IntegerConstant_t,
                        make_Integer_t, n_args, 4, dim.loc);
                    dim.m_start = make_ConstantWithKind(make_IntegerConstant_t,
                        make_Integer_t, 0, 4, dim.loc);
                    dims.push_back(al, dim);
                    type = ASRUtils::make_Array_t_util(al, x.base.base.loc, type, dims.p, dims.size(),
                        ASR::abiType::Source, false, ASR::array_physical_typeType::PointerToDataArray, true);
                    for( size_t i = 0; i < n_args; i++ ) {
                        m_args[i] = CastingUtil::perform_casting(m_args[i], ASRUtils::expr_type(m_args[i]),
                                        ASRUtils::type_get_past_array(type), al, x.base.base.loc);
                    }
                    tmp = ASR::make_ArrayConstant_t(al, x.base.base.loc, m_args, n_args, type, ASR::arraystorageType::RowMajor);
                } else {
                    throw SemanticError("array accepts only list for now, got " +
                                        ASRUtils::type_to_str(type) + " type.", x.base.base.loc);
                }
                return;
            } else if( call_name == "deepcopy" ) {
                parse_args(x, args);
                if( args.size() != 1 ) {
                    throw SemanticError("deepcopy only accepts one argument, found " +
                                        std::to_string(args.size()) + " instead.",
                                        x.base.base.loc);
                }
                tmp = (ASR::asr_t*) args[0].m_value;
                return ;
            } else if( call_name == "sizeof" ) {

                if( x.n_args + x.n_keywords != 1 ) {
                    throw SemanticError("sizeof only accepts one argument, found " +
                                        std::to_string(x.n_args + x.n_keywords) + " instead.",
                                        x.base.base.loc);
                }
                bool is_allocatable = false;
                ASR::ttype_t* arg_type = ast_expr_to_asr_type(x.base.base.loc, *x.m_args[0],
                                is_allocatable, false);
                ASR::expr_t* arg = nullptr;
                if( !arg_type ) {
                    visit_expr(*x.m_args[0]);
                    arg = ASRUtils::EXPR(tmp);
                }
                if( arg ) {
                    if( ASR::is_a<ASR::Var_t>(*arg) ) {
                        ASR::Var_t* arg_Var = ASR::down_cast<ASR::Var_t>(arg);
                        ASR::symbol_t* arg_Var_m_v = ASRUtils::symbol_get_past_external(arg_Var->m_v);
                        if( ASR::is_a<ASR::Variable_t>(*arg_Var_m_v) ) {
                            // TODO: Import the underlying struct if arg_type is of Struct type
                            // Ideally if a variable of struct type is being imported then its underlying type
                            // should also be imported automatically. However, the naming of the
                            // underlying struct type might lead to collisions, so importing the type
                            // here seems like a better choice. Should be done later when the case arises.
                            arg_type = ASR::down_cast<ASR::Variable_t>(arg_Var_m_v)->m_type;
                        } else {
                            throw SemanticError("Symbol " + std::to_string(arg_Var_m_v->type) +
                                                " is not yet supported in sizeof.",
                                                x.base.base.loc);
                        }
                    }
                }
                ASR::ttype_t* size_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                tmp = ASR::make_SizeOfType_t(al, x.base.base.loc,
                                             arg_type, size_type, nullptr);
                return ;
            }  else if( call_name == "field" ) {
                if (x.n_args != 0) {
                    throw SemanticError("'field' expects only keyword arguments", x.base.base.loc);
                }

                if (x.n_keywords != 1) {
                    throw SemanticError("'field' expects one keyword argument", x.base.base.loc);
                }

                args.reserve(al, 1);
                visit_expr_list(x.m_args, x.n_args, args);

                if( std::string(x.m_keywords[0].m_arg) != "default_factory" && std::string(x.m_keywords[0].m_arg) != "default" ) {
                    throw SemanticError("Unrecognised keyword argument, " +
                        std::string(x.m_keywords[0].m_arg), x.base.base.loc);
                }

                if ( std::string(x.m_keywords[0].m_arg) == "default_factory") {
                    if (!AST::is_a<AST::Lambda_t>(*x.m_keywords[0].m_value)) {
                        throw SemanticError("Only lambda functions currently supported as default_factory value", x.base.base.loc);
                    }

                    AST::Lambda_t* lambda_fn = AST::down_cast<AST::Lambda_t>(x.m_keywords[0].m_value);
                    this->visit_expr(*lambda_fn->m_body);
                } else {
                    // field has default argument provided
                    this->visit_expr(*x.m_keywords[0].m_value);
                }
                return ;
            } else if(
                        call_name == "f64" ||
                        call_name == "f32" ||
                        call_name == "i64" ||
                        call_name == "i32" ||
                        call_name == "i16" ||
                        call_name == "i8"  ||
                        call_name == "u64" ||
                        call_name == "u32" ||
                        call_name == "u16" ||
                        call_name == "u8"  ||
                        call_name == "c32" ||
                        call_name == "c64" ||
                        call_name == "S"
                    ) {
                parse_args(x, args);
                ASR::ttype_t* target_type = nullptr;
                if( call_name == "i8" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 1));
                } else if( call_name == "i16" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 2));
                } else if( call_name == "i32" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4));
                } else if( call_name == "i64" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc, 8));
                } else if( call_name == "u8" ) {
                    target_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, x.base.base.loc, 1));
                } else if( call_name == "u16" ) {
                    target_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, x.base.base.loc, 2));
                } else if( call_name == "u32" ) {
                    target_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, x.base.base.loc, 4));
                } else if( call_name == "u64" ) {
                    target_type = ASRUtils::TYPE(ASR::make_UnsignedInteger_t(al, x.base.base.loc, 8));
                } else if( call_name == "f32" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, 4));
                } else if( call_name == "f64" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc, 8));
                } else if( call_name == "c32" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, 4));
                } else if( call_name == "c64" ) {
                    target_type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc, 8));
                } else if( call_name == "S" ) {
                    target_type = ASRUtils::TYPE(ASR::make_SymbolicExpression_t(al, x.base.base.loc));
                }
                ASR::expr_t* arg = args[0].m_value;
                if (ASR::is_a<ASR::UnsignedInteger_t>(*target_type)) {
                    int64_t value_int;
                    if( ASRUtils::extract_value(ASRUtils::expr_value(arg), value_int) ) {
                        if (value_int < 0) {
                            throw SemanticError("Cannot cast negative value to unsigned integer ",
                                                x.base.base.loc);
                        }
                    }
                }
                cast_helper(target_type, arg, x.base.base.loc, true);
                tmp = (ASR::asr_t*) arg;
                return ;
            } else if (intrinsic_node_handler.is_present(call_name)) {
                parse_args(x, args);
                tmp = intrinsic_node_handler.get_intrinsic_node(call_name, al,
                                        x.base.base.loc, args);
                return;
            } else {
                // The function was not found and it is not intrinsic
                throw SemanticError("Function '" + call_name + "' is not declared and not intrinsic",
                    x.base.base.loc);
            }
            } // end of "comment"
        }

        parse_args(x, args);
        tmp = make_call_helper(al, s, current_scope, args, call_name, x.base.base.loc,
                               x.m_args, x.n_args, x.m_keywords, x.n_keywords);
    }

    void visit_Global(const AST::Global_t &/*x*/) {
        tmp = nullptr;
    }
};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al, LocationManager &lm, const AST::Module_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit, bool main_module, std::string module_name,
        std::map<int, ASR::symbol_t*> &ast_overload,
        bool allow_implicit_casting)
{
    BodyVisitor b(al, lm, unit, diagnostics, main_module, module_name, ast_overload, allow_implicit_casting);
    try {
        b.visit_Module(ast);
    } catch (const SemanticError &e) {
        Error error;
        diagnostics.diagnostics.push_back(e.d);
        return error;
    } catch (const SemanticAbort &) {
        Error error;
        return error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    return tu;
}

std::string get_parent_dir(const std::string &path) {
    int idx = path.size()-1;
    while (idx >= 0 && path[idx] != '/' && path[idx] != '\\') idx--;
    if (idx == -1) {
        return "";
    }
    return path.substr(0,idx);
}

Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al, LocationManager &lm, SymbolTable* symtab,
    AST::ast_t &ast, diag::Diagnostics &diagnostics, CompilerOptions &compiler_options,
    bool main_module, std::string module_name, std::string file_path, bool allow_implicit_casting)
{
    std::map<int, ASR::symbol_t*> ast_overload;
    std::string parent_dir = get_parent_dir(file_path);
    AST::Module_t *ast_m = AST::down_cast2<AST::Module_t>(&ast);

    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, lm, symtab, *ast_m, diagnostics, main_module, module_name,
        ast_overload, parent_dir, compiler_options.import_paths, allow_implicit_casting);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    if (compiler_options.po.dump_all_passes) {
        std::ofstream outfile ("pass_00_initial_asr_01.clj");
        outfile << ";; ASR after SymbolTable Visitor\n" << pickle(*tu, false, true, compiler_options.po.with_intrinsic_mods) << "\n";
        outfile.close();
    }
    if (compiler_options.po.dump_fortran) {
        LCompilers::Result<std::string> fortran_code = LCompilers::asr_to_fortran(*tu, diagnostics, false, 4);
        if (!fortran_code.ok) {
            LCOMPILERS_ASSERT(diagnostics.has_error());
            throw LCompilersException("Fortran code could not be generated after symbol_table_visitor");
        }
        std::ofstream outfile ("pass_fortran_00_initial_code_01.f90");
        outfile << "! Fortran code after SymbolTable Visitor\n" << fortran_code.result << "\n";
        outfile.close();
    }
#if defined(WITH_LFORTRAN_ASSERT)
        if (!asr_verify(*tu, true, diagnostics)) {
            std::cerr << diagnostics.render2();
            throw LCompilersException("Verify failed");
        };
#endif

    if (!compiler_options.symtab_only) {
        auto res2 = body_visitor(al, lm, *ast_m, diagnostics, unit, main_module, module_name,
            ast_overload, allow_implicit_casting);
        if (res2.ok) {
            tu = res2.result;
        } else {
            return res2.error;
        }
        if (compiler_options.po.dump_all_passes) {
            std::ofstream outfile ("pass_00_initial_asr_02.clj");
            outfile << ";; Initial ASR after Body Visitor\n" << pickle(*tu, false, true, compiler_options.po.with_intrinsic_mods) << "\n";
            outfile.close();
        }
        if (compiler_options.po.dump_fortran) {
            LCompilers::Result<std::string> fortran_code = LCompilers::asr_to_fortran(*tu, diagnostics, false, 4);
            if (!fortran_code.ok) {
                LCOMPILERS_ASSERT(diagnostics.has_error());
                throw LCompilersException("Fortran code could not be generated after body_visitor");
            }
            std::ofstream outfile ("pass_fortran_00_initial_code_02.f90");
            outfile << "! Fortran code after Body Visitor\n" << fortran_code.result << "\n";
            outfile.close();
        }
#if defined(WITH_LFORTRAN_ASSERT)
        diag::Diagnostics diagnostics;
        if (!asr_verify(*tu, true, diagnostics)) {
            std::cerr << diagnostics.render2();
            throw LCompilersException("Verify failed");
        };
#endif
    }

    if (main_module && !compiler_options.po.disable_main) {
        // If it is a main module, turn it into a program
        // Note: we can modify this behavior for interactive mode later

        Vec<ASR::stmt_t*> prog_body;
        prog_body.reserve(al, 1);
        SetChar prog_dep;
        prog_dep.reserve(al, 1);
        SymbolTable *program_scope = al.make_new<SymbolTable>(tu->m_symtab);

        std::string mod_name = "__main__";
        ASR::symbol_t *mod_sym = tu->m_symtab->resolve_symbol(mod_name);
        LCOMPILERS_ASSERT(mod_sym);
        ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(mod_sym);
        LCOMPILERS_ASSERT(mod);
        std::vector<ASR::asr_t*> tmp_vec;
        get_calls_to_global_init_and_stmts(al, tu->base.base.loc, program_scope, mod, tmp_vec);

        for (auto i:tmp_vec) {
            prog_body.push_back(al, ASRUtils::STMT(i));
        }

        if (prog_body.size() > 0) {
            prog_dep.push_back(al, s2c(al, mod_name));
        }

        std::string prog_name = "main_program";
        ASR::asr_t *prog = ASR::make_Program_t(
            al, tu->base.base.loc,
            /* a_symtab */ program_scope,
            /* a_name */ s2c(al, prog_name),
            prog_dep.p,
            prog_dep.n,
            /* a_body */ prog_body.p,
            /* n_body */ prog_body.n);
        tu->m_symtab->add_symbol(prog_name, ASR::down_cast<ASR::symbol_t>(prog));

        #if defined(WITH_LFORTRAN_ASSERT)
            diag::Diagnostics diagnostics;
            if (!asr_verify(*tu, true, diagnostics)) {
                std::cerr << diagnostics.render2();
                throw LCompilersException("Verify failed");
            };
        #endif
    }

    return tu;
}

} // namespace LCompilers::LPython
