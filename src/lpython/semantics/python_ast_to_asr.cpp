#include <fstream>
#include <iostream>
#include <map>
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
#include <libasr/pass/global_stmts_program.h>
#include <libasr/pass/instantiate_template.h>
#include <libasr/modfile.h>

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


namespace LFortran::LPython {

namespace CastingUtil {

    // Data structure which contains priorities for
    // intrinsic types defined in ASR
    const std::map<ASR::ttypeType, int>& type2weight = {
        {ASR::ttypeType::Complex, 4},
        {ASR::ttypeType::Real, 3},
        {ASR::ttypeType::Integer, 2},
        {ASR::ttypeType::Logical, 1}
    };

    // Data structure which contains casting rules for non-equal
    // intrinsic types defined in ASR
    const std::map<std::pair<ASR::ttypeType, ASR::ttypeType>, ASR::cast_kindType>& type_rules = {
        {std::make_pair(ASR::ttypeType::Complex, ASR::ttypeType::Real), ASR::cast_kindType::ComplexToReal},
        {std::make_pair(ASR::ttypeType::Complex, ASR::ttypeType::Logical), ASR::cast_kindType::ComplexToLogical},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Complex), ASR::cast_kindType::RealToComplex},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Integer), ASR::cast_kindType::RealToInteger},
        {std::make_pair(ASR::ttypeType::Real, ASR::ttypeType::Logical), ASR::cast_kindType::RealToLogical},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Complex), ASR::cast_kindType::IntegerToComplex},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Real), ASR::cast_kindType::IntegerToReal},
        {std::make_pair(ASR::ttypeType::Integer, ASR::ttypeType::Logical), ASR::cast_kindType::IntegerToLogical},
        {std::make_pair(ASR::ttypeType::Logical, ASR::ttypeType::Real), ASR::cast_kindType::LogicalToReal},
        {std::make_pair(ASR::ttypeType::Logical, ASR::ttypeType::Integer), ASR::cast_kindType::LogicalToInteger},
    };

    // Data structure which contains casting rules for equal intrinsic
    // types but with different kinds.
    const std::map<ASR::ttypeType, ASR::cast_kindType>& kind_rules = {
        {ASR::ttypeType::Complex, ASR::cast_kindType::ComplexToComplex},
        {ASR::ttypeType::Real, ASR::cast_kindType::RealToReal},
        {ASR::ttypeType::Integer, ASR::cast_kindType::IntegerToInteger}
    };

    int get_type_priority(ASR::ttypeType type) {
        if( type2weight.find(type) == type2weight.end() ) {
            return -1;
        }

        return type2weight.at(type);
    }

    int get_src_dest(ASR::expr_t* left_expr, ASR::expr_t* right_expr,
                      ASR::expr_t*& src_expr, ASR::expr_t*& dest_expr,
                      ASR::ttype_t*& src_type, ASR::ttype_t*& dest_type,
                      bool is_assign) {
        ASR::ttype_t* left_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(left_expr));
        ASR::ttype_t* right_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(right_expr));
        if( ASRUtils::check_equal_type(left_type, right_type) ||
            ASRUtils::is_character(*left_type) || ASRUtils::is_character(*right_type) ) {
            return 2;
        }
        if( is_assign ) {
            if( ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
                throw SemanticError("Assigning integer to float is not supported",
                                    right_expr->base.loc);
            }
            if ( ASRUtils::is_complex(*left_type) && !ASRUtils::is_complex(*right_type)) {
                throw SemanticError("Assigning non-complex to complex is not supported",
                        right_expr->base.loc);
            }
            dest_expr = left_expr, dest_type = left_type;
            src_expr = right_expr, src_type = right_type;
            return 1;
        }

        int casted_expr_signal = 2;
        ASR::ttypeType left_Type = left_type->type, right_Type = right_type->type;
        int left_kind = ASRUtils::extract_kind_from_ttype_t(left_type);
        int right_kind = ASRUtils::extract_kind_from_ttype_t(right_type);
        int left_priority = get_type_priority(left_Type);
        int right_priority = get_type_priority(right_Type);
        if( left_priority > right_priority ) {
            src_expr = right_expr, src_type = right_type;
            dest_expr = left_expr, dest_type = left_type;
            casted_expr_signal = 1;
        } else if( left_priority < right_priority ) {
            src_expr = left_expr, src_type = left_type;
            dest_expr = right_expr, dest_type = right_type;
            casted_expr_signal = 0;
        } else {
            if( left_kind > right_kind ) {
                src_expr = right_expr, src_type = right_type;
                dest_expr = left_expr, dest_type = left_type;
                casted_expr_signal = 1;
            } else if( left_kind < right_kind ) {
                src_expr = left_expr, src_type = left_type;
                dest_expr = right_expr, dest_type = right_type;
                casted_expr_signal = 0;
            } else {
                return 2;
            }
        }

        return casted_expr_signal;
    }

    ASR::expr_t* perform_casting(ASR::expr_t* expr, ASR::ttype_t* src,
                                 ASR::ttype_t* dest, Allocator& al) {
        ASR::ttypeType src_type = src->type;
        ASR::ttypeType dest_type = dest->type;
        ASR::cast_kindType cast_kind;
        if( src_type == dest_type ) {
            if( kind_rules.find(src_type) == kind_rules.end() ) {
                return expr;
            }
            cast_kind = kind_rules.at(src_type);
        } else {
            std::pair<ASR::ttypeType, ASR::ttypeType> cast_key = std::make_pair(src_type, dest_type);
            if( type_rules.find(cast_key) == type_rules.end() ) {
                return expr;
            }
            cast_kind = type_rules.at(cast_key);
        }
        return ASRUtils::EXPR(ASRUtils::make_Cast_t_value(al, expr->base.loc, expr,
                                                          cast_kind, dest));
    }
}

int save_pyc_files(const LFortran::ASR::TranslationUnit_t &u,
                       std::string infile) {
    LFORTRAN_ASSERT(LFortran::asr_verify(u));
    std::string modfile_binary = LFortran::save_pycfile(u);

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
LFortran::Result<std::string> get_full_path(const std::string &filename,
        const std::string &runtime_library_dir, std::string& input,
        bool &ltypes) {
    ltypes = false;
    bool status = read_file(filename, input);
    if (status) {
        return filename;
    } else {
        std::string filename_intrinsic = runtime_library_dir + "/" + filename;
        status = read_file(filename_intrinsic, input);
        if (status) {
            return filename_intrinsic;
        } else {
            // If this is `ltypes`, do a special lookup
            if (filename == "ltypes.py") {
                filename_intrinsic = runtime_library_dir + "/ltypes/" + filename;
                status = read_file(filename_intrinsic, input);
                if (status) {
                    ltypes = true;
                    return filename_intrinsic;
                } else {
                    return LFortran::Error();
                }
            } else if (startswith(filename, "numpy.py")) {
                filename_intrinsic = runtime_library_dir + "/lpython_intrinsic_" + filename;
                status = read_file(filename_intrinsic, input);
                if (status) {
                    return filename_intrinsic;
                } else {
                    return LFortran::Error();
                }
            } else {
                return LFortran::Error();
            }
        }
    }
}

bool set_module_path(std::string infile0, std::vector<std::string> &rl_path,
                     std::string& infile, std::string& path_used, std::string& input,
                     bool& ltypes) {
    for (auto path: rl_path) {
        Result<std::string> rinfile = get_full_path(infile0, path, input, ltypes);
        if (rinfile.ok) {
            infile = rinfile.result;
            path_used = path;
            return true;
        }
    }
    return false;
}

ASR::TranslationUnit_t* compile_module_till_asr(Allocator& al,
    std::vector<std::string> &rl_path, std::string infile,
    const Location &loc,
    const std::function<void (const std::string &, const Location &)> err) {
    // TODO: diagnostic should be an argument to this function
    diag::Diagnostics diagnostics;
    Result<AST::ast_t*> r = parse_python_file(al, rl_path[0], infile,
        diagnostics, false);
    if (!r.ok) {
        err("The file '" + infile + "' failed to parse", loc);
    }
    LFortran::LPython::AST::ast_t* ast = r.result;

    // Convert the module from AST to ASR
    LFortran::LocationManager lm;
    lm.in_filename = infile;
    Result<ASR::TranslationUnit_t*> r2 = python_ast_to_asr(al, *ast,
        diagnostics, false, true, false, infile);
    // TODO: Uncomment once a check is added for ensuring
    // that module.py file hasn't changed between
    // builds.
    // save_pyc_files(*r2.result, infile + "c");
    std::string input;
    read_file(infile, input);
    CompilerOptions compiler_options;
    std::cerr << diagnostics.render(input, lm, compiler_options);
    if (!r2.ok) {
        LFORTRAN_ASSERT(diagnostics.has_error())
        return nullptr; // Error
    }

    return r2.result;
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            std::vector<std::string> &rl_path,
                            bool &ltypes,
                            const std::function<void (const std::string &, const Location &)> err) {
    if( module_name == "copy" ) {
        return nullptr;
    }
    ltypes = false;
    LFORTRAN_ASSERT(symtab);
    if (symtab->get_scope().find(module_name) != symtab->get_scope().end()) {
        ASR::symbol_t *m = symtab->get_symbol(module_name);
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            err("The symbol '" + module_name + "' is not a module", loc);
        }
    }
    LFORTRAN_ASSERT(symtab->parent == nullptr);

    // Parse the module `module_name`.py to AST
    std::string infile0 = module_name + ".py";
    std::string infile0c = infile0 + "c";
    std::string path_used = "", infile;
    bool compile_module = true;
    ASR::TranslationUnit_t* mod1 = nullptr;
    std::string input;
    bool found = set_module_path(infile0c, rl_path, infile,
                                 path_used, input, ltypes);
    if( !found ) {
        input.clear();
        found = set_module_path(infile0, rl_path, infile,
                                path_used, input, ltypes);
    } else {
        mod1 = load_pycfile(al, input, false);
        fix_external_symbols(*mod1, *ASRUtils::get_tu_symtab(symtab));
        LFORTRAN_ASSERT(asr_verify(*mod1));
        compile_module = false;
    }

    if (!found) {
        err("Could not find the module '" + infile0 + "'", loc);
    }
    if (ltypes) return nullptr;

    if( compile_module ) {
        mod1 = compile_module_till_asr(al, rl_path, infile, loc, err);
    }

    // insert into `symtab`
    std::vector<std::pair<std::string, ASR::Module_t*>> children_modules;
    ASRUtils::extract_module_python(*mod1, children_modules, module_name);
    ASR::Module_t* mod2 = nullptr;
    for( auto& a: children_modules ) {
        std::string a_name = a.first;
        ASR::Module_t* a_mod = a.second;
        if( a_name == module_name ) {
            a_mod->m_name = s2c(al, module_name);
            a_mod->m_intrinsic = intrinsic;
            mod2 = a_mod;
        }
        symtab->add_symbol(a_name, (ASR::symbol_t*)a_mod);
        a_mod->m_symtab->parent = symtab;
    }
    if (intrinsic) {
        // TODO: I think we should just store intrinsic once, in the module
        // itself
        // Mark each function as intrinsic also
        for (auto &item : mod2->m_symtab->get_scope()) {
            if (ASR::is_a<ASR::Function_t>(*item.second)) {
                ASR::Function_t *s = ASR::down_cast<ASR::Function_t>(item.second);
                if (s->m_abi == ASR::abiType::Source) {
                    s->m_abi = ASR::abiType::Intrinsic;
                }
                if (s->n_body == 0) {
                    std::string name = s->m_name;
                    if (name == "ubound" || name == "lbound") {
                        s->m_deftype = ASR::deftypeType::Interface;
                    }
                }
            }
        }
    }

    // and return it
    return mod2;
}

ASR::symbol_t* import_from_module(Allocator &al, ASR::Module_t *m, SymbolTable *current_scope,
                std::string mname, std::string cur_sym_name, std::string new_sym_name,
                const Location &loc) {
    ASR::symbol_t *t = m->m_symtab->resolve_symbol(cur_sym_name);
    if (!t) {
        throw SemanticError("The symbol '" + cur_sym_name + "' not found in the module '" + mname + "'",
                loc);
    }
    if (current_scope->get_scope().find(cur_sym_name) != current_scope->get_scope().end()) {
        throw SemanticError(cur_sym_name + " already defined", loc);
    }
    if (ASR::is_a<ASR::Function_t>(*t)) {
        ASR::Function_t *mfn = ASR::down_cast<ASR::Function_t>(t);
        // `mfn` is the Function in a module. Now we construct
        // an ExternalSymbol that points to it.
        Str name;
        name.from_str(al, new_sym_name);
        char *cname = name.c_str(al);
        ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
            al, mfn->base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ cname,
            (ASR::symbol_t*)mfn,
            m->m_name, nullptr, 0, mfn->m_name,
            ASR::accessType::Public
            );
        return ASR::down_cast<ASR::symbol_t>(fn);
    } else if (ASR::is_a<ASR::Variable_t>(*t)) {
        ASR::Variable_t *mv = ASR::down_cast<ASR::Variable_t>(t);
        // `mv` is the Variable in a module. Now we construct
        // an ExternalSymbol that points to it.
        Str name;
        name.from_str(al, new_sym_name);
        char *cname = name.c_str(al);
        ASR::asr_t *v = ASR::make_ExternalSymbol_t(
            al, mv->base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ cname,
            (ASR::symbol_t*)mv,
            m->m_name, nullptr, 0, mv->m_name,
            ASR::accessType::Public
            );
        return ASR::down_cast<ASR::symbol_t>(v);
    } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
        ASR::GenericProcedure_t *gt = ASR::down_cast<ASR::GenericProcedure_t>(t);
        Str name;
        name.from_str(al, new_sym_name);
        char *cname = name.c_str(al);
        ASR::asr_t *v = ASR::make_ExternalSymbol_t(
            al, gt->base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ cname,
            (ASR::symbol_t*)gt,
            m->m_name, nullptr, 0, gt->m_name,
            ASR::accessType::Public
            );
        return ASR::down_cast<ASR::symbol_t>(v);
    } else {
        throw SemanticError("Only Subroutines, Functions and Variables are currently supported in 'import'",
            loc);
    }
    LFORTRAN_ASSERT(false);
    return nullptr;
}


template <class Derived>
class CommonVisitor : public AST::BaseVisitor<Derived> {
public:
    diag::Diagnostics &diag;


    ASR::asr_t *tmp;

    /*
    If `tmp` is not null, then `tmp_vec` is ignored and `tmp` is used as the only result (statement or
    expression). If `tmp` is null, then `tmp_vec` is used to return any number of statements:
    0 (no statement returned), 1 (redundant, one should use `tmp` for that), 2, 3, ... etc.
    */
    std::vector<ASR::asr_t *> tmp_vec;

    Allocator &al;
    SymbolTable *current_scope;
    // The current_module contains the current module that is being visited;
    // this is used to append to the module dependencies if needed
    ASR::Module_t *current_module = nullptr;
    Vec<char *> current_module_dependencies;
    // True for the main module, false for every other one
    // The main module is stored directly in TranslationUnit, other modules are Modules
    bool main_module;
    PythonIntrinsicProcedures intrinsic_procedures;
    ProceduresDatabase procedures_db;
    AttributeHandler attr_handler;
    IntrinsicNodeHandler intrinsic_node_handler;
    std::map<int, ASR::symbol_t*> &ast_overload;
    std::string parent_dir;
    Vec<ASR::stmt_t*> *current_body;
    ASR::ttype_t* ann_assign_target_type;

    std::map<std::string, int> generic_func_nums;
    std::map<std::string, std::map<std::string, ASR::ttype_t*>> generic_func_subs;

    CommonVisitor(Allocator &al, SymbolTable *symbol_table,
            diag::Diagnostics &diagnostics, bool main_module,
            std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir)
        : diag{diagnostics}, al{al}, current_scope{symbol_table}, main_module{main_module},
            ast_overload{ast_overload}, parent_dir{parent_dir},
            current_body{nullptr}, ann_assign_target_type{nullptr} {
        current_module_dependencies.reserve(al, 4);
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
        LFORTRAN_ASSERT(intrinsic_procedures.is_intrinsic(remote_sym))
        std::string module_name = intrinsic_procedures.get_module(remote_sym, loc);

        SymbolTable *tu_symtab = ASRUtils::get_tu_symtab(current_scope);
        std::string rl_path = get_runtime_library_dir();
        std::vector<std::string> paths = {rl_path, parent_dir};
        bool ltypes;
        ASR::Module_t *m = load_module(al, tu_symtab, module_name,
                loc, true, paths,
                ltypes,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); }
                );
        LFORTRAN_ASSERT(!ltypes)

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
        ASR::asr_t *fn = ASR::make_ExternalSymbol_t(
            al, t->base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ fn_name,
            t,
            m->m_name, nullptr, 0, fn_name,
            ASR::accessType::Private
            );
        std::string sym = fn_name;

        current_scope->add_symbol(sym, ASR::down_cast<ASR::symbol_t>(fn));
        ASR::symbol_t *v = ASR::down_cast<ASR::symbol_t>(fn);

        // Now we need to add the module `m` with the intrinsic function
        // into the current module dependencies
        if (current_module) {
            // We are in body visitor, the module is already constructed
            // and available as current_module.
            // Add the module `m` to current module dependencies
            Vec<char*> vec;
            vec.from_pointer_n_copy(al, current_module->m_dependencies,
                        current_module->n_dependencies);
            if (!present(vec, m->m_name)) {
                vec.push_back(al, m->m_name);
                current_module->m_dependencies = vec.p;
                current_module->n_dependencies = vec.size();
            }
        } else {
            // We are in the symtab visitor or body visitor and we are
            // constructing a module, so current_module is not available yet
            // (the current_module_dependencies is not used in body visitor)
            if (!present(current_module_dependencies, m->m_name)) {
                current_module_dependencies.push_back(al, m->m_name);
            }
        }
        return v;
    }

    void handle_attribute(ASR::expr_t *s, std::string attr_name,
                const Location &loc, Vec<ASR::expr_t*> &args) {
        tmp = attr_handler.get_attribute(s, attr_name, al, loc, args, diag);
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
                                                 orig_args);
        for( size_t i = 0; i < exprs.size(); i++ ) {
            ASR::expr_t* expri = exprs[i];
            if (expri) {
                expr_duplicator.success = true;
                ASR::expr_t* expri_copy = expr_duplicator.duplicate_expr(expri);
                LFORTRAN_ASSERT(expr_duplicator.success);
                arg_replacer.current_expr = &expri_copy;
                arg_replacer.replace_expr(expri_copy);
                exprs[i] = expri_copy;
            }
        }
    }

    ASR::ttype_t* handle_return_type(ASR::ttype_t *return_type, const Location &loc,
                                     Vec<ASR::call_arg_t>& args,
                                     ASR::Function_t* f=nullptr) {
        // Rebuild the return type if needed and make FunctionCalls use ExternalSymbol
        std::vector<ASR::expr_t*> func_calls;
        switch( return_type->type ) {
            case ASR::ttypeType::Character: {
                ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(return_type);
                func_calls.push_back(t->m_len_expr);
                fill_expr_in_ttype_t(func_calls, t->m_dims, t->n_dims);
                fix_exprs_ttype_t(func_calls, args, f);
                Vec<ASR::dimension_t> new_dims;
                new_dims.reserve(al, t->n_dims);
                for( size_t i = 1; i < func_calls.size(); i += 2 ) {
                    ASR::dimension_t new_dim;
                    new_dim.loc = func_calls[i]->base.loc;
                    new_dim.m_start = func_calls[i];
                    new_dim.m_length = func_calls[i + 1];
                    new_dims.push_back(al, new_dim);
                }
                int64_t a_len = t->m_len;
                if( func_calls[0] ) {
                    a_len = ASRUtils::extract_len<SemanticError>(func_calls[0], loc);
                }
                return ASRUtils::TYPE(ASR::make_Character_t(al, loc, t->m_kind, a_len, func_calls[0], new_dims.p, new_dims.size()));
            }
            case ASR::ttypeType::Integer: {
                ASR::Integer_t *t = ASR::down_cast<ASR::Integer_t>(return_type);
                fill_expr_in_ttype_t(func_calls, t->m_dims, t->n_dims);
                fix_exprs_ttype_t(func_calls, args, f);
                Vec<ASR::dimension_t> new_dims;
                new_dims.reserve(al, t->n_dims);
                for( size_t i = 0; i < func_calls.size(); i += 2 ) {
                    ASR::dimension_t new_dim;
                    new_dim.loc = func_calls[i]->base.loc;
                    new_dim.m_start = func_calls[i];
                    new_dim.m_length = func_calls[i + 1];
                    new_dims.push_back(al, new_dim);
                }
                return ASRUtils::TYPE(ASR::make_Integer_t(al, loc, t->m_kind, new_dims.p, new_dims.size()));
            }
            case ASR::ttypeType::Real: {
                ASR::Real_t *t = ASR::down_cast<ASR::Real_t>(return_type);
                fill_expr_in_ttype_t(func_calls, t->m_dims, t->n_dims);
                fix_exprs_ttype_t(func_calls, args, f);
                Vec<ASR::dimension_t> new_dims;
                new_dims.reserve(al, t->n_dims);
                for( size_t i = 0; i < func_calls.size(); i += 2 ) {
                    ASR::dimension_t new_dim;
                    new_dim.loc = func_calls[i]->base.loc;
                    new_dim.m_start = func_calls[i];
                    new_dim.m_length = func_calls[i + 1];
                    new_dims.push_back(al, new_dim);
                }
                return ASRUtils::TYPE(ASR::make_Real_t(al, loc, t->m_kind, new_dims.p, new_dims.size()));
                break;
            }
            default: {
                return return_type;
            }
        }
        return nullptr;
    }

    void visit_expr_list(AST::expr_t** exprs, size_t n,
                         Vec<ASR::expr_t*> exprs_vec) {
        LFORTRAN_ASSERT(exprs_vec.reserve_called);
        for( size_t i = 0; i < n; i++ ) {
            this->visit_expr(*exprs[i]);
            exprs_vec.push_back(al, ASRUtils::EXPR(tmp));
        }
    }

    void visit_expr_list(AST::expr_t** exprs, size_t n,
                         Vec<ASR::call_arg_t>& call_args_vec) {
        LFORTRAN_ASSERT(call_args_vec.reserve_called);
        for( size_t i = 0; i < n; i++ ) {
            this->visit_expr(*exprs[i]);
            ASR::expr_t* expr = nullptr;
            ASR::call_arg_t arg;
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
                         ASR::Function_t* orig_func, const Location& call_loc,
                         bool raise_error=true) {
        LFORTRAN_ASSERT(call_args_vec.reserve_called);

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

    void visit_expr_list(Vec<ASR::call_arg_t>& exprs, size_t n,
                         Vec<ASR::expr_t*>& exprs_vec) {
        LFORTRAN_ASSERT(exprs_vec.reserve_called);
        for( size_t i = 0; i < n; i++ ) {
            exprs_vec.push_back(al, exprs[i].m_value);
        }
    }

    void visit_expr_list_with_cast(ASR::expr_t** m_args, size_t n_args,
                                   Vec<ASR::call_arg_t>& call_args_vec,
                                   Vec<ASR::call_arg_t>& args) {
        LFORTRAN_ASSERT(call_args_vec.reserve_called);
        for (size_t i = 0; i < n_args; i++) {
            ASR::call_arg_t c_arg;
            c_arg.loc = args[i].loc;
            c_arg.m_value = args[i].m_value;
            cast_helper(m_args[i], c_arg.m_value, true);
            call_args_vec.push_back(al, c_arg);
        }
    }

    ASR::ttype_t* get_type_from_var_annotation(std::string var_annotation,
        const Location& loc, Vec<ASR::dimension_t>& dims,
        AST::expr_t** m_args=nullptr, size_t n_args=0) {
        ASR::ttype_t* type = nullptr;
        if (var_annotation == "i8") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                1, dims.p, dims.size()));
        } else if (var_annotation == "i16") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                2, dims.p, dims.size()));
        } else if (var_annotation == "i32") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "i64") {
            type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                8, dims.p, dims.size()));
        } else if (var_annotation == "f32") {
            type = ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "f64") {
            type = ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                8, dims.p, dims.size()));
        } else if (var_annotation == "c32") {
            type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "c64") {
            type = ASRUtils::TYPE(ASR::make_Complex_t(al, loc,
                8, dims.p, dims.size()));
        } else if (var_annotation == "str") {
            type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, -2, nullptr, dims.p, dims.size()));
        } else if (var_annotation == "bool") {
            type = ASRUtils::TYPE(ASR::make_Logical_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "CPtr") {
            type = ASRUtils::TYPE(ASR::make_CPtr_t(al, loc));
        } else if (var_annotation == "pointer") {
            LFORTRAN_ASSERT(n_args == 1);
            AST::expr_t* underlying_type = m_args[0];
            type = ast_expr_to_asr_type(underlying_type->base.loc, *underlying_type);
            type = ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, type));
        } else {
            ASR::symbol_t *s = current_scope->resolve_symbol(var_annotation);
            if (s) {
                if (ASR::is_a<ASR::Variable_t>(*s)) {
                    ASR::Variable_t *var_sym = ASR::down_cast<ASR::Variable_t>(s);
                    if (var_sym->m_type->type == ASR::ttypeType::TypeParameter) {
                        ASR::TypeParameter_t *type_param = ASR::down_cast<ASR::TypeParameter_t>(var_sym->m_type);
                        return ASRUtils::TYPE(ASR::make_TypeParameter_t(al, loc,
                            type_param->m_param, dims.p, dims.size(), type_param->m_rt, type_param->n_rt));
                    }
                } else {
                    ASR::symbol_t *der_sym = ASRUtils::symbol_get_past_external(s);
                    if (der_sym && der_sym->type == ASR::symbolType::DerivedType) {
                        return ASRUtils::TYPE(ASR::make_Derived_t(al, loc, der_sym, dims.p, dims.size()));
                    }
                }
            }
            throw SemanticError("Unsupported type annotation: " + var_annotation, loc);
        }
        return type;
    }


    // Function to create appropriate call based on symbol type. If it is external
    // generic symbol then it changes the name accordingly.
    ASR::asr_t* make_call_helper(Allocator &al, ASR::symbol_t* s, SymbolTable *current_scope,
                    Vec<ASR::call_arg_t> args, std::string call_name, const Location &loc,
                    bool ignore_return_value=false, AST::expr_t** pos_args=nullptr, size_t n_pos_args=0,
                    AST::keyword_t* kwargs=nullptr, size_t n_kwargs=0) {
        if (intrinsic_node_handler.is_present(call_name)) {
            return intrinsic_node_handler.get_intrinsic_node(call_name, al, loc,
                    args, ann_assign_target_type);
        }
        ASR::symbol_t *s_generic = nullptr, *stemp = s;
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
                                         args, orig_func, loc, false) ) {
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
            while (symtab->parent != nullptr && symtab->get_scope().find(local_sym) == symtab->get_scope().end()) {
                symtab = symtab->parent;
            }
            if (symtab->get_scope().find(local_sym) == symtab->get_scope().end()) {
                LFORTRAN_ASSERT(ASR::is_a<ASR::ExternalSymbol_t>(*stemp));
                std::string mod_name = ASR::down_cast<ASR::ExternalSymbol_t>(stemp)->m_module_name;
                ASR::symbol_t *mt = symtab->get_symbol(mod_name);
                ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(mt);
                stemp = import_from_module(al, m, symtab, mod_name,
                                    remote_sym, local_sym, loc);
                LFORTRAN_ASSERT(ASR::is_a<ASR::ExternalSymbol_t>(*stemp));
                symtab->add_symbol(local_sym, stemp);
                s = ASRUtils::symbol_get_past_external(stemp);
            } else {
                stemp = symtab->get_symbol(local_sym);
            }
        }
        if (ASR::is_a<ASR::Function_t>(*s)) {
            ASR::Function_t *func = ASR::down_cast<ASR::Function_t>(s);
            if( n_kwargs > 0 && !is_generic_procedure ) {
                args.reserve(al, n_pos_args + n_kwargs);
                visit_expr_list(pos_args, n_pos_args, kwargs, n_kwargs,
                                args, func, loc);
            }
            if (func->n_type_params > 0) {
                std::map<std::string, ASR::ttype_t*> subs;
                for (size_t i=0; i<args.size(); i++) {
                    ASR::ttype_t *param_type = ASRUtils::expr_type(func->m_args[i]);
                    ASR::ttype_t *arg_type = ASRUtils::expr_type(args[i].m_value);
                    subs = check_type_substitution(subs, param_type, arg_type, loc);
                }

                ASR::symbol_t *t = get_generic_function(subs, *func);
                std::string new_call_name = call_name;
                if (ASR::is_a<ASR::Function_t>(*t)) {
                    new_call_name = (ASR::down_cast<ASR::Function_t>(t))->m_name;
                }
                return make_call_helper(al, t, current_scope, args, new_call_name, loc);
            }
            if (ASR::down_cast<ASR::Function_t>(s)->m_return_var != nullptr) {
                ASR::ttype_t *a_type = nullptr;
                if( func->m_elemental && args.size() == 1 &&
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
                Vec<ASR::call_arg_t> args_new;
                args_new.reserve(al, func->n_args);
                visit_expr_list_with_cast(func->m_args, func->n_args, args_new, args);
                ASR::asr_t* func_call_asr = ASR::make_FunctionCall_t(al, loc, stemp,
                                                s_generic, args_new.p, args_new.size(),
                                                a_type, value, nullptr);
                if( ignore_return_value ) {
                    std::string dummy_ret_name = current_scope->get_unique_name("__lcompilers_dummy");
                    ASR::asr_t* variable_asr = ASR::make_Variable_t(al, loc, current_scope,
                                                    s2c(al, dummy_ret_name), ASR::intentType::Local,
                                                    nullptr, nullptr, ASR::storage_typeType::Default,
                                                    a_type, ASR::abiType::Source, ASR::accessType::Public,
                                                    ASR::presenceType::Required, false);
                    ASR::symbol_t* variable_sym = ASR::down_cast<ASR::symbol_t>(variable_asr);
                    current_scope->add_symbol(dummy_ret_name, variable_sym);
                    ASR::expr_t* variable_var = ASRUtils::EXPR(ASR::make_Var_t(al, loc, variable_sym));
                    return ASR::make_Assignment_t(al, loc, variable_var, ASRUtils::EXPR(func_call_asr), nullptr);
                } else {
                    return func_call_asr;
                }
            } else {
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
                Vec<ASR::call_arg_t> args_new;
                args_new.reserve(al, func->n_args);
                visit_expr_list_with_cast(func->m_args, func->n_args, args_new, args);
                return ASR::make_SubroutineCall_t(al, loc, stemp,
                    s_generic, args_new.p, args_new.size(), nullptr);
            }
        } else if(ASR::is_a<ASR::DerivedType_t>(*s)) {
            Vec<ASR::expr_t*> args_new;
            args_new.reserve(al, args.size());
            visit_expr_list(args, args.size(), args_new);
            ASR::DerivedType_t* derivedtype = ASR::down_cast<ASR::DerivedType_t>(s);
            for( size_t i = 0; i < std::min(args.size(), derivedtype->n_members); i++ ) {
                std::string member_name = derivedtype->m_members[i];
                ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(
                                                derivedtype->m_symtab->resolve_symbol(member_name));
                ASR::expr_t* arg_new_i = args_new[i];
                cast_helper(member_var->m_type, arg_new_i);
                args_new.p[i] = arg_new_i;
            }
            ASR::ttype_t* der_type = ASRUtils::TYPE(ASR::make_Derived_t(al, loc, s, nullptr, 0));
            return ASR::make_DerivedTypeConstructor_t(al, loc, s, args_new.p, args_new.size(), der_type, nullptr);
        } else {
            throw SemanticError("Unsupported call type for " + call_name, loc);
        }
    }

    std::map<std::string, ASR::ttype_t*> check_type_substitution(std::map<std::string, ASR::ttype_t*> subs,
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
        if (ASR::is_a<ASR::TypeParameter_t>(*param_type)) {
            ASR::TypeParameter_t *tp = ASR::down_cast<ASR::TypeParameter_t>(param_type);
            if (!check_type_restriction(arg_type, tp)) {
                diag.add(diag::Diagnostic(
                    "Argument type does not match parameter's restriction",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("Type mismatch", {param_type->base.loc, arg_type->base.loc})
                    }
                ));
                throw SemanticAbort();
            }
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
                        subs[param_name] = ASRUtils::duplicate_type_without_dims(al, arg_type);
                    } else {
                        throw SemanticError("Inconsistent type subsititution for array type", loc);
                    }
                } else {
                    subs[param_name] = ASRUtils::duplicate_type(al, arg_type);
                }
            }
        }
        return subs;
    }

    // TODO: Think also about dimensions
    bool check_type_restriction(ASR::ttype_t *expr_type, ASR::TypeParameter_t *tp) {
        for (size_t i=0; i<tp->n_rt; i++) {
            ASR::Restriction_t *restriction = ASR::down_cast<ASR::Restriction_t>(tp->m_rt[i]);
            switch (restriction->m_rt) {
                case (ASR::traitType::SupportsZero): {
                    switch (expr_type->type) {
                        case (ASR::ttypeType::Integer): { continue; }
                        case (ASR::ttypeType::Real): { continue; }
                        default: return false;
                    }
                }
                case (ASR::traitType::SupportsPlus): {
                    switch (expr_type->type) {
                        case (ASR::ttypeType::Integer): { continue; }
                        case (ASR::ttypeType::Real): { continue; }
                        case (ASR::ttypeType::Character): { continue; }
                        default: return false;
                    }
                }
                case (ASR::traitType::Divisible): {
                    switch (expr_type -> type) {
                        case (ASR::ttypeType::Integer): { continue; }
                        case (ASR::ttypeType::Real): { continue; }
                        default: return false;
                    }
                }
                case (ASR::traitType::Any): {
                    continue;
                }
                default: continue;
            }
        }
        return true;
    }

    ASR::symbol_t* get_generic_function(std::map<std::string, ASR::ttype_t*> subs,
            ASR::Function_t &func) {
        int new_function_num;
        ASR::symbol_t *t;
        std::string func_name = func.m_name;
        if (generic_func_nums.find(func_name) != generic_func_nums.end()) {
            new_function_num = generic_func_nums[func_name];
            for (int i=0; i<generic_func_nums[func_name]; i++) {
                std::string generic_func_name = "__lpython_generic_" + func_name + "_" + std::to_string(i);
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
        std::string new_func_name = "__lpython_generic_" + func_name + "_" + std::to_string(new_function_num);
        generic_func_subs[new_func_name] = subs;
        t = pass_instantiate_generic_function(al, subs, current_scope, new_func_name, func);
        return t;
    }

    void fill_dims_for_asr_type(Vec<ASR::dimension_t>& dims,
                                ASR::expr_t* value, const Location& loc) {
        ASR::dimension_t dim;
        dim.loc = loc;
        if (ASR::is_a<ASR::IntegerConstant_t>(*value) ||
            ASR::is_a<ASR::Var_t>(*value)) {
            ASR::ttype_t *itype = ASRUtils::expr_type(value);
            ASR::expr_t* one = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 1, itype));
            ASR::expr_t* zero = ASRUtils::EXPR(ASR::make_IntegerConstant_t(al, loc, 0, itype));
            ASR::expr_t* comptime_val = nullptr;
            int64_t value_int = -1;
            ASRUtils::extract_value(ASRUtils::expr_value(value), value_int);
            if( value_int != -1 ) {
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
                fill_dims_for_asr_type(dims, value, loc);
            }
        } else {
            throw SemanticError("Only Integer, `:` or identifier in [] in Subscript supported for now in annotation"
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

    // Convert Python AST type annotation to an ASR type
    // Examples:
    // i32, i64, f32, f64
    // f64[256], i32[:]
    ASR::ttype_t * ast_expr_to_asr_type(const Location &loc, const AST::expr_t &annotation) {
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 4);
        AST::expr_t** m_args = nullptr; size_t n_args = 0;

        std::string var_annotation;
        if (AST::is_a<AST::Name_t>(annotation)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(&annotation);
            var_annotation = n->m_id;
        } else if (AST::is_a<AST::Subscript_t>(annotation)) {
            AST::Subscript_t *s = AST::down_cast<AST::Subscript_t>(&annotation);
            if (AST::is_a<AST::Name_t>(*s->m_value)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(s->m_value);
                var_annotation = n->m_id;
            } else {
                throw SemanticError("Only Name in Subscript supported for now in annotation",
                    loc);
            }

            if (var_annotation == "tuple") {
                Vec<ASR::ttype_t*> types;
                types.reserve(al, 4);
                if (AST::is_a<AST::Name_t>(*s->m_slice)) {
                    types.push_back(al, ast_expr_to_asr_type(loc, *s->m_slice));
                } else if (AST::is_a<AST::Tuple_t>(*s->m_slice)) {
                    AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                    for (size_t i=0; i<t->n_elts; i++) {
                        types.push_back(al, ast_expr_to_asr_type(loc, *t->m_elts[i]));
                    }
                } else {
                    throw SemanticError("Only Name or Tuple in Subscript supported for now in `tuple` annotation",
                        loc);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Tuple_t(al, loc,
                    types.p, types.size()));
                return type;
            } else if (var_annotation == "set") {
                if (AST::is_a<AST::Name_t>(*s->m_slice)) {
                    ASR::ttype_t *type = ast_expr_to_asr_type(loc, *s->m_slice);
                    return ASRUtils::TYPE(ASR::make_Set_t(al, loc, type));
                } else {
                    throw SemanticError("Only Name in Subscript supported for now in `set`"
                        " annotation", loc);
                }
            } else if (var_annotation == "list") {
                ASR::ttype_t *type = nullptr;
                if (AST::is_a<AST::Name_t>(*s->m_slice) || AST::is_a<AST::Subscript_t>(*s->m_slice)) {
                    type = ast_expr_to_asr_type(loc, *s->m_slice);
                    return ASRUtils::TYPE(ASR::make_List_t(al, loc, type));
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
                    ASR::ttype_t *key_type = ast_expr_to_asr_type(loc, *t->m_elts[0]);
                    ASR::ttype_t *value_type = ast_expr_to_asr_type(loc, *t->m_elts[1]);
                    return ASRUtils::TYPE(ASR::make_Dict_t(al, loc, key_type, value_type));
                } else {
                    throw SemanticError("`dict` annotation must have 2 elements: types of"
                        " both keys and values", loc);
                }
            } else if (var_annotation == "Pointer") {
                ASR::ttype_t *type = ast_expr_to_asr_type(loc, *s->m_slice);
                return ASRUtils::TYPE(ASR::make_Pointer_t(al, loc, type));
            } else {
                if (AST::is_a<AST::Slice_t>(*s->m_slice)) {
                    ASR::dimension_t dim;
                    dim.loc = loc;
                    dim.m_start = nullptr;
                    dim.m_length = nullptr;
                    dims.push_back(al, dim);
                } else if( is_runtime_array(s->m_slice) ) {
                    AST::Tuple_t* tuple_multidim = AST::down_cast<AST::Tuple_t>(s->m_slice);
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
                    this->visit_expr(*s->m_slice);
                    ASR::expr_t *value = ASRUtils::EXPR(tmp);
                    fill_dims_for_asr_type(dims, value, loc);
                }
            }
        } else {
            throw SemanticError("Only Name, Subscript, and Call supported for now in annotation of annotated assignment.",
                loc);
        }

        return get_type_from_var_annotation(var_annotation, annotation.base.loc, dims, m_args, n_args);
    }

    ASR::expr_t *index_add_one(const Location &loc, ASR::expr_t *idx) {
        // Add 1 to the index `idx`, assumes `idx` is of type Integer 4
        ASR::expr_t *comptime_value = nullptr;
        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
            4, nullptr, 0));
        ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, loc, 1, a_type));
        return ASRUtils::EXPR(ASR::make_IntegerBinOp_t(al, loc, idx,
            ASR::binopType::Add, constant_one, a_type, comptime_value));
    }

    void cast_helper(ASR::expr_t*& left, ASR::expr_t*& right, bool is_assign) {
        bool no_cast = ((ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(left)) &&
                        ASR::is_a<ASR::Var_t>(*left)) ||
                        (ASR::is_a<ASR::Pointer_t>(*ASRUtils::expr_type(right)) &&
                        ASR::is_a<ASR::Var_t>(*right)));
        ASR::ttype_t *right_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(right));
        ASR::ttype_t *left_type = ASRUtils::type_get_past_pointer(ASRUtils::expr_type(left));
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
            ASR::ttype_t* dest_type = ASRUtils::TYPE(ASR::make_Integer_t(al, left_type->base.loc,
                                            4, nullptr, 0));
            left = CastingUtil::perform_casting(left, left_type, dest_type, al);
            right = CastingUtil::perform_casting(right, right_type, dest_type, al);
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
                                                dest_type, al);
        if( casted_expression_signal == 0 ) {
            left = src_expr;
            right = dest_expr;
        } else if( casted_expression_signal == 1 ) {
            left = dest_expr;
            right = src_expr;
        }
    }

    void cast_helper(ASR::ttype_t* dest_type, ASR::expr_t*& src_expr) {
        ASR::ttype_t* src_type = ASRUtils::expr_type(src_expr);
        if( ASRUtils::check_equal_type(src_type, dest_type) ) {
            return ;
        }
        src_expr = CastingUtil::perform_casting(src_expr, src_type,
                                                dest_type, al);
    }

    void make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
                            ASR::binopType op, const Location &loc, bool floordiv) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;
        ASR::expr_t *overloaded = nullptr;

        bool right_is_int = ASRUtils::is_character(*left_type) && ASRUtils::is_integer(*right_type);
        bool left_is_int = ASRUtils::is_integer(*left_type) && ASRUtils::is_character(*right_type);

        // Handle normal division in python with reals
        if (op == ASR::binopType::Div) {
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
            // Floor div operation in python using (`//`)
            if (floordiv) {
                bool both_int = (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type));
                if (both_int) {
                    cast_helper(left, right, false);
                    dest_type = ASRUtils::expr_type(left);
                } else {
                    dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                        8, nullptr, 0));
                    if (ASRUtils::is_integer(*left_type)) {
                        left = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                            al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type));
                    }
                    if (ASRUtils::is_integer(*right_type)) {
                        right = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                            al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type));
                    }
                    cast_helper(left, right, false);
                    dest_type = ASRUtils::expr_type(left);
                }
                if (ASRUtils::expr_value(right) != nullptr) {
                    if (ASRUtils::is_integer(*right_type)) {
                        int8_t value = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                        if (value == 0) {
                            diag.add(diag::Diagnostic(
                                "integer division by zero is not allowed",
                                diag::Level::Error, diag::Stage::Semantic, {
                                    diag::Label("integer division by zero",
                                            {right->base.loc})
                                })
                            );
                            throw SemanticAbort();
                        }
                    } else if (ASRUtils::is_real(*right_type)) {
                        double value = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                        if (value == 0.0) {
                            diag.add(diag::Diagnostic(
                                "float floor division by zero is not allowed",
                                diag::Level::Error, diag::Stage::Semantic, {
                                    diag::Label("float floor division by zero",
                                            {right->base.loc})
                                })
                            );
                            throw SemanticAbort();
                        }
                    }
                }
                ASR::symbol_t *fn_div = resolve_intrinsic_function(loc, "_lpython_floordiv");
                Vec<ASR::call_arg_t> args;
                args.reserve(al, 1);
                ASR::call_arg_t arg1, arg2;
                arg1.loc = left->base.loc;
                arg2.loc = right->base.loc;
                arg1.m_value = left;
                arg2.m_value = right;
                args.push_back(al, arg1);
                args.push_back(al, arg2);
                tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_floordiv", loc);
                return;

            } else { // real divison in python using (`/`)
                dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                    8, nullptr, 0));
                if (ASRUtils::is_integer(*left_type)) {
                    left = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                        al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type));
                } else if (ASR::is_a<ASR::TypeParameter_t>(*left_type)) {
                    left = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                        al, left->base.loc, left, ASR::cast_kindType::TemplateToReal, dest_type));
                }
                if (ASRUtils::is_integer(*right_type)) {
                    if (ASRUtils::expr_value(right) != nullptr) {
                        int64_t val = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::expr_value(right))->m_n;
                        if (val == 0) {
                            diag.add(diag::Diagnostic(
                                "division by zero is not allowed",
                                diag::Level::Error, diag::Stage::Semantic, {
                                    diag::Label("division by zero",
                                            {right->base.loc})
                                })
                            );
                            throw SemanticAbort();
                        }
                    }
                    right = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                        al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type));
                } else if (ASRUtils::is_real(*right_type)) {
                    if (ASRUtils::expr_value(right) != nullptr) {
                        double val = ASR::down_cast<ASR::RealConstant_t>(ASRUtils::expr_value(right))->m_r;
                        if (val == 0.0) {
                            diag.add(diag::Diagnostic(
                                "float division by zero is not allowed",
                                diag::Level::Error, diag::Stage::Semantic, {
                                    diag::Label("float division by zero",
                                            {right->base.loc})
                                })
                            );
                            throw SemanticAbort();
                        }
                    }
                }
            }
        } else if((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type) ||
                    ASRUtils::is_complex(*left_type) || ASRUtils::is_logical(*left_type)) &&
                (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type) ||
                    ASRUtils::is_complex(*right_type) || ASRUtils::is_logical(*right_type))) {
            cast_helper(left, right, false);
            dest_type = ASRUtils::expr_type(left);
        } else if ((right_is_int || left_is_int) && op == ASR::binopType::Mul) {
            // string repeat
            int64_t left_int = 0, right_int = 0, dest_len = 0;
            if (right_is_int) {
                ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
                LFORTRAN_ASSERT(left_type2->n_dims == 0);
                right_int = ASR::down_cast<ASR::IntegerConstant_t>(
                                                   ASRUtils::expr_value(right))->m_n;
                dest_len = left_type2->m_len * right_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, left_type2->m_kind,
                        dest_len, nullptr, nullptr, 0));
            } else if (left_is_int) {
                ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
                LFORTRAN_ASSERT(right_type2->n_dims == 0);
                left_int = ASR::down_cast<ASR::IntegerConstant_t>(
                                                   ASRUtils::expr_value(left))->m_n;
                dest_len = right_type2->m_len * left_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, right_type2->m_kind,
                        dest_len, nullptr, nullptr, 0));
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
                LFORTRAN_ASSERT((int64_t)strlen(result) == dest_len)
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
            ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
            ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
            LFORTRAN_ASSERT(left_type2->n_dims == 0);
            LFORTRAN_ASSERT(right_type2->n_dims == 0);
            dest_type = ASR::down_cast<ASR::ttype_t>(
                    ASR::make_Character_t(al, loc, left_type2->m_kind,
                    left_type2->m_len + right_type2->m_len, nullptr, nullptr, 0));
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* left_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::StringConstant_t>(
                                        ASRUtils::expr_value(right))->m_s;
                char* result;
                std::string result_s = std::string(left_value) + std::string(right_value);
                result = s2c(al, result_s);
                LFORTRAN_ASSERT((int64_t)strlen(result) == ASR::down_cast<ASR::Character_t>(dest_type)->m_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_StringConstant_t(
                    al, loc, result, dest_type));
            }
            tmp = ASR::make_StringConcat_t(al, loc, left, right, dest_type, value);
            return;
        } else if (ASR::is_a<ASR::TypeParameter_t>(*left_type) && ASR::is_a<ASR::TypeParameter_t>(*right_type)) {
            ASR::TypeParameter_t *left_param = ASR::down_cast<ASR::TypeParameter_t>(left_type);
            ASR::TypeParameter_t *right_param = ASR::down_cast<ASR::TypeParameter_t>(right_type);
            if (op == ASR::binopType::Add) {
                if (ASRUtils::has_trait(left_param, ASR::traitType::SupportsPlus) &&
                        ASRUtils::has_trait(right_param, ASR::traitType::SupportsPlus)) {
                    dest_type = left_type;
                } else {
                    throw SemanticError("Both type variables must support addition operation", loc);
                }
            }
            if (op == ASR::binopType::Div) {
                if (ASRUtils::has_trait(left_param, ASR::traitType::Divisible)) {
                    dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc, 8, nullptr, 0));
                } else {
                    throw SemanticError("The left expression of the division must be support division operation", loc);
                }
            }
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

        if (ASRUtils::is_integer(*dest_type)) {

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
                    default: { LFORTRAN_ASSERT(false); } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                    al, loc, result, dest_type));
            }

            tmp = ASR::make_IntegerBinOp_t(al, loc, left, op, right, dest_type, value);

        } else if (ASRUtils::is_real(*dest_type)) {

            if (op == ASR::binopType::BitAnd || op == ASR::binopType::BitOr || op == ASR::binopType::BitXor ||
                op == ASR::binopType::BitLShift || op == ASR::binopType::BitRShift) {
                throw SemanticError("Unsupported binary operation on floats: '" + ASRUtils::binop_to_str_python(op) + "'", loc);
            }

            cast_helper(left, right, false);
            dest_type = ASRUtils::expr_type(right);
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
                    default: { LFORTRAN_ASSERT(false); }
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
                    default: { LFORTRAN_ASSERT(false); }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ComplexConstant_t(al, loc,
                        std::real(result), std::imag(result), dest_type));
            }

            tmp = ASR::make_ComplexBinOp_t(al, loc, left, op, right, dest_type, value);

        } else if (ASRUtils::is_generic(*dest_type)) {
            tmp = ASR::make_TemplateBinOp_t(al, loc, left, op, right, dest_type, value);
        }


        if (overloaded != nullptr) {
            tmp = ASR::make_OverloadedBinOp_t(al, loc, left, op, right, dest_type, value, overloaded);
        }
    }

    bool is_dataclass(AST::expr_t** decorators, size_t n) {
        if( n != 1 ) {
            return false;
        }

        AST::expr_t* decorator = decorators[0];
        if( !AST::is_a<AST::Name_t>(*decorator) ) {
            return false;
        }

        AST::Name_t* dec_name = AST::down_cast<AST::Name_t>(decorator);
        return std::string(dec_name->m_id) == "dataclass";
    }

    void visit_AnnAssignUtil(const AST::AnnAssign_t& x, std::string& var_name,
                             bool wrap_derived_type_in_pointer=false) {
        ASR::ttype_t *type = ast_expr_to_asr_type(x.base.base.loc, *x.m_annotation);
        ASR::ttype_t* ann_assign_target_type_copy = ann_assign_target_type;
        ann_assign_target_type = type;
        if( ASR::is_a<ASR::Derived_t>(*type) &&
            wrap_derived_type_in_pointer ) {
            type = ASRUtils::TYPE(ASR::make_Pointer_t(al, type->base.loc, type));
        }

        ASR::expr_t *value = nullptr;
        ASR::expr_t *init_expr = nullptr;
        tmp = nullptr;
        if (x.m_value) {
            this->visit_expr(*x.m_value);
        }
        if (tmp) {
            value = ASRUtils::EXPR(tmp);
            cast_helper(type, value);
            if (!ASRUtils::check_equal_type(type, ASRUtils::expr_type(value))) {
                std::string ltype = ASRUtils::type_to_str_python(type);
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
            // Set compile time to value to nullptr
            // Once constant variables are supported
            // in LPython set value according to the
            // nature of the variable (nullptr if non-constant,
            // otherwise ASRUtils::expr_value(init_expr).
            value = nullptr;
        }
        ASR::intentType s_intent = ASRUtils::intent_local;
        ASR::storage_typeType storage_type =
                ASR::storage_typeType::Default;
        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                s2c(al, var_name), s_intent, init_expr, value, storage_type, type,
                current_procedure_abi_type, s_access, s_presence,
                value_attr);
        ASR::symbol_t* v_sym = ASR::down_cast<ASR::symbol_t>(v);
        // Convert initialisation at declaration to assignment
        // only for non-global variables. For global variables
        // keep relying on `m_symbolic_value`.
        if( init_expr && current_body) {
            ASR::expr_t* v_expr = ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc, v_sym));
            cast_helper(v_expr, init_expr, true);
            ASR::asr_t* assign = ASR::make_Assignment_t(al, x.base.base.loc, v_expr,
                                                        init_expr, nullptr);
            current_body->push_back(al, ASRUtils::STMT(assign));
            ASR::Variable_t* v_variable = ASR::down_cast<ASR::Variable_t>(v_sym);
            v_variable->m_symbolic_value = nullptr;
            v_variable->m_value = nullptr;
        }
        current_scope->add_symbol(var_name, v_sym);

        tmp = nullptr;
        ann_assign_target_type = ann_assign_target_type_copy;
    }

    void visit_ClassDef(const AST::ClassDef_t& x) {
        std::string x_m_name = x.m_name;
        if( current_scope->resolve_symbol(x_m_name) ) {
            return ;
        }
        if( !is_dataclass(x.m_decorator_list, x.n_decorator_list) ) {
            throw SemanticError("Only dataclass decorated classes are supported.",
                                x.base.base.loc);
        }

        if( x.n_bases > 0 ) {
            throw SemanticError("Inheritance in classes isn't supported yet.",
                                x.base.base.loc);
        }

        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);

        Vec<char*> member_names;
        member_names.reserve(al, x.n_body);
        for( size_t i = 0; i < x.n_body; i++ ) {
            LFORTRAN_ASSERT(AST::is_a<AST::AnnAssign_t>(*x.m_body[i]));
            AST::AnnAssign_t* ann_assign = AST::down_cast<AST::AnnAssign_t>(x.m_body[i]);
            LFORTRAN_ASSERT(AST::is_a<AST::Name_t>(*ann_assign->m_target));
            AST::Name_t *n = AST::down_cast<AST::Name_t>(ann_assign->m_target);
            std::string var_name = n->m_id;
            visit_AnnAssignUtil(*ann_assign, var_name, true);
            member_names.push_back(al, n->m_id);
        }
        ASR::symbol_t* class_type = ASR::down_cast<ASR::symbol_t>(ASR::make_DerivedType_t(al,
                                        x.base.base.loc, current_scope, x.m_name,
                                        member_names.p, member_names.size(),
                                        ASR::abiType::Source, ASR::accessType::Public,
                                        nullptr));
        current_scope = parent_scope;
        current_scope->add_symbol(std::string(x.m_name), class_type);
    }

    void add_name(const Location &loc) {
        std::string var_name = "__name__";
        std::string var_value;
        if (main_module) {
            var_value = "__main__";
        } else {
            // TODO: put the actual module name here
            var_value = "__non_main__";
        }
        size_t s_size = var_value.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, s_size, nullptr, nullptr, 0));
        ASR::expr_t *value = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
            loc, s2c(al, var_value), type));
        ASR::expr_t *init_expr = value;

        ASR::intentType s_intent = ASRUtils::intent_local;
        ASR::storage_typeType storage_type =
                ASR::storage_typeType::Default;
        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), s_intent, init_expr, value, storage_type, type,
                current_procedure_abi_type, s_access, s_presence,
                value_attr);
        current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));
    }

    void add_lpython_version(const Location &loc) {
        std::string var_name = "__LPYTHON_VERSION__";
        std::string var_value = LFORTRAN_VERSION;
        size_t s_size = var_value.size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, s_size, nullptr, nullptr, 0));
        ASR::expr_t *value = ASRUtils::EXPR(ASR::make_StringConstant_t(al,
            loc, s2c(al, var_value), type));
        ASR::expr_t *init_expr = value;

        ASR::intentType s_intent = ASRUtils::intent_local;
        ASR::storage_typeType storage_type =
                ASR::storage_typeType::Default;
        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::presenceType s_presence = ASR::presenceType::Required;
        bool value_attr = false;
        ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                s2c(al, var_name), s_intent, init_expr, value, storage_type, type,
                current_procedure_abi_type, s_access, s_presence,
                value_attr);
        current_scope->add_symbol(var_name, ASR::down_cast<ASR::symbol_t>(v));
    }

    void visit_Name(const AST::Name_t &x) {
        std::string name = x.m_id;
        ASR::symbol_t *s = current_scope->resolve_symbol(name);
        if (s) {
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else if (name == "i32" || name == "i64" || name == "f32" || name == "f64") {
            int64_t i = -1;
            ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                    4, nullptr, 0));
            tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, i, type);
        } else if (name == "__name__") {
            // __name__ was not declared yet in this scope, so we
            // declare it first:
            add_name(x.base.base.loc);
            // And now resolve it:
            ASR::symbol_t *s = current_scope->resolve_symbol(name);
            LFORTRAN_ASSERT(s);
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else if (name == "__LPYTHON_VERSION__") {
            add_lpython_version(x.base.base.loc);
            ASR::symbol_t *s = current_scope->resolve_symbol(name);
            LFORTRAN_ASSERT(s);
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else {
            throw SemanticError("Variable '" + name + "' not declared",
                x.base.base.loc);
        }
    }

    void visit_NamedExpr(const AST::NamedExpr_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = ASRUtils::EXPR(tmp);
        ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
        LFORTRAN_ASSERT(ASRUtils::check_equal_type(target_type, value_type));
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
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, i, type);
    }

    void visit_ConstantFloat(const AST::ConstantFloat_t &x) {
        double f = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_RealConstant_t(al, x.base.base.loc, f, type);
    }

    void visit_ConstantComplex(const AST::ConstantComplex_t &x) {
        double re = x.m_re, im = x.m_im;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_ComplexConstant_t(al, x.base.base.loc, re, im, type);
    }

    void visit_ConstantStr(const AST::ConstantStr_t &x) {
        char *s = x.m_value;
        size_t s_size = std::string(s).size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                1, s_size, nullptr, nullptr, 0));
        tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s, type);
    }

    void visit_ConstantBool(const AST::ConstantBool_t &x) {
        bool b = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
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
        LFORTRAN_ASSERT(
            ASRUtils::check_equal_type(ASRUtils::expr_type(lhs), ASRUtils::expr_type(rhs)));
        ASR::expr_t *value = nullptr;
        ASR::ttype_t *dest_type = ASRUtils::expr_type(lhs);

        if (ASRUtils::expr_value(lhs) != nullptr && ASRUtils::expr_value(rhs) != nullptr) {

            LFORTRAN_ASSERT(ASR::is_a<ASR::Logical_t>(*dest_type));
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
        ASR::binopType op;
        std::string op_name = "";
        switch (x.m_op) {
            case (AST::operatorType::Add) : { op = ASR::binopType::Add; break; }
            case (AST::operatorType::Sub) : { op = ASR::binopType::Sub; break; }
            case (AST::operatorType::Mult) : { op = ASR::binopType::Mul; break; }
            case (AST::operatorType::Div) : { op = ASR::binopType::Div; break; }
            case (AST::operatorType::FloorDiv) : {op = ASR::binopType::Div; break;}
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
        Vec<ASR::call_arg_t> args;
        args.reserve(al, 2);
        ASR::call_arg_t arg1, arg2;
        arg1.loc = left->base.loc;
        arg1.m_value = left;
        args.push_back(al, arg1);
        arg2.loc = right->base.loc;
        arg2.m_value = right;
        args.push_back(al, arg2);
        if (op_name != "") {
            ASR::symbol_t *fn_mod = resolve_intrinsic_function(x.base.base.loc, op_name);
            tmp = make_call_helper(al, fn_mod, current_scope, args, op_name, x.base.base.loc);
            return;
        }
        bool floordiv = (x.m_op == AST::operatorType::FloorDiv);
        make_BinOp_helper(left, right, op, x.base.base.loc, floordiv);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = ASRUtils::EXPR(tmp);
        ASR::ttype_t *operand_type = ASRUtils::expr_type(operand);
        ASR::ttype_t *dest_type = operand_type;
        ASR::ttype_t *logical_type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            4, nullptr, 0));
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
            }
            else if (ASRUtils::is_real(*operand_type)) {
                throw SemanticError("Unary operator '~' not supported for floats",
                    x.base.base.loc);
            }
            else if (ASRUtils::is_logical(*operand_type)) {
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
            }
            else if (ASRUtils::is_complex(*operand_type)) {
                throw SemanticError("Unary operator '~' not supported for complex type",
                    x.base.base.loc);
            }

        }
        else if (x.m_op == AST::unaryopType::Not) {

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
            }
            else if (ASRUtils::is_real(*operand_type)) {
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
            }
            else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                        al, x.base.base.loc, !op_value, logical_type));
                }
            }
            else if (ASRUtils::is_complex(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
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
            }

            tmp = ASR::make_LogicalNot_t(al, x.base.base.loc, logical_arg, logical_type, value);
            return;

        }
        else if (x.m_op == AST::unaryopType::UAdd) {

            if (ASRUtils::is_integer(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    int64_t op_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                            ASRUtils::expr_value(operand))->m_n;
                    tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, op_value, operand_type);
                }
            }
            else if (ASRUtils::is_real(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    double op_value = ASR::down_cast<ASR::RealConstant_t>(
                                            ASRUtils::expr_value(operand))->m_r;
                    tmp = ASR::make_RealConstant_t(al, x.base.base.loc, op_value, operand_type);
                }
            }
            else if (ASRUtils::is_logical(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    bool op_value = ASR::down_cast<ASR::LogicalConstant_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                    tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, +op_value, int_type);
                    return;
                }
                tmp = ASR::make_Cast_t(al, x.base.base.loc, operand, ASR::cast_kindType::LogicalToInteger,
                        int_type, value);
            }
            else if (ASRUtils::is_complex(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    ASR::ComplexConstant_t *c = ASR::down_cast<ASR::ComplexConstant_t>(
                                        ASRUtils::expr_value(operand));
                    std::complex<double> op_value(c->m_re, c->m_im);
                    tmp = ASR::make_ComplexConstant_t(al, x.base.base.loc,
                        std::real(op_value), std::imag(op_value), operand_type);
                }
            }
            return;

        }
        else if (x.m_op == AST::unaryopType::USub) {

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
            }
            else if (ASRUtils::is_real(*operand_type)) {
                if (ASRUtils::expr_value(operand) != nullptr) {
                    double op_value = ASR::down_cast<ASR::RealConstant_t>(
                                            ASRUtils::expr_value(operand))->m_r;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_RealConstant_t(
                        al, x.base.base.loc, -op_value, operand_type));
                }
                tmp = ASR::make_RealUnaryMinus_t(al, x.base.base.loc, operand,
                                                 operand_type, value);
                return;
            }
            else if (ASRUtils::is_logical(*operand_type)) {
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
            }
            else if (ASRUtils::is_complex(*operand_type)) {
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
        LFORTRAN_ASSERT(ASRUtils::check_equal_type(ASRUtils::expr_type(body),
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
                if (!ASRUtils::is_integer(*ASRUtils::expr_type(ASRUtils::EXPR(tmp)))) {
                    throw SemanticError("slice indices must be integers or None", tmp->loc);
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
                  !ASR::is_a<ASR::Dict_t>(*type)) {
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
                    "Type mismatch in index, Index should be of integer type",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("type mismatch (found: '" + fnd + "', expected: 'i32')",
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
                tmp = make_ListItem_t(al, loc, value, index,
                                      ASR::down_cast<ASR::List_t>(type)->m_type, nullptr);
                return false;
            } else if (ASR::is_a<ASR::Tuple_t>(*type)) {
                index = ASRUtils::EXPR(tmp);
                int i = ASR::down_cast<ASR::IntegerConstant_t>(ASRUtils::EXPR(tmp))->m_n;
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
            type = ASRUtils::duplicate_type(al, type, &empty_dims);
            tmp = ASR::make_ArrayItem_t(al, x.base.base.loc, v_Var, args.p,
                        args.size(), type, nullptr);
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


    SymbolTableVisitor(Allocator &al, SymbolTable *symbol_table,
        diag::Diagnostics &diagnostics, bool main_module,
        std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir)
      : CommonVisitor(al, symbol_table, diagnostics, main_module, ast_overload, parent_dir), is_derived_type{false} {}


    ASR::symbol_t* resolve_symbol(const Location &loc, const std::string &sub_name) {
        SymbolTable *scope = current_scope;
        ASR::symbol_t *sub = scope->resolve_symbol(sub_name);
        if (!sub) {
            throw SemanticError("Symbol '" + sub_name + "' not declared", loc);
        }
        return sub;
    }

    void visit_Module(const AST::Module_t &x) {
        if (!current_scope) {
            current_scope = al.make_new<SymbolTable>(nullptr);
        }
        LFORTRAN_ASSERT(current_scope != nullptr);
        global_scope = current_scope;

        // Create the TU early, so that asr_owner is set, so that
        // ASRUtils::get_tu_symtab() can be used, which has an assert
        // for asr_owner.
        ASR::asr_t *tmp0 = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);

        ASR::Module_t* module_sym = nullptr;

        if (!main_module) {
            // Main module goes directly to TranslationUnit.
            // Every other module goes into a Module.
            SymbolTable *parent_scope = current_scope;
            current_scope = al.make_new<SymbolTable>(parent_scope);


            std::string mod_name = "__main__";
            ASR::asr_t *tmp1 = ASR::make_Module_t(al, x.base.base.loc,
                                        /* a_symtab */ current_scope,
                                        /* a_name */ s2c(al, mod_name),
                                        nullptr,
                                        0,
                                        false, false);

            if (parent_scope->get_scope().find(mod_name) != parent_scope->get_scope().end()) {
                throw SemanticError("Module '" + mod_name + "' already defined", tmp1->loc);
            }
            module_sym = ASR::down_cast<ASR::Module_t>(ASR::down_cast<ASR::symbol_t>(tmp1));
            parent_scope->add_symbol(mod_name, ASR::down_cast<ASR::symbol_t>(tmp1));
        }
        current_module_dependencies.reserve(al, 1);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }
        if( module_sym ) {
            module_sym->m_dependencies = current_module_dependencies.p;
            module_sym->n_dependencies = current_module_dependencies.size();
        }
        if (!overload_defs.empty()) {
            create_GenericProcedure(x.base.base.loc);
        }
        global_scope = nullptr;
        tmp = tmp0;
    }

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.m_args.n_args);
        current_procedure_abi_type = ASR::abiType::Source;
        bool current_procedure_interface = false;
        bool overload = false;
        Vec<ASR::ttype_t*> tps;
        tps.reserve(al, x.m_args.n_args);
        bool vectorize = false, is_inline = false;
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
                    } else if (name == "overload") {
                        overload = true;
                    } else if (name == "interface") {
                        // TODO: Implement @interface
                    } else if (name == "vectorize") {
                        vectorize = true;
                    } else if (name == "inline") {
                        is_inline = true;
                    } else {
                        throw SemanticError("Decorator: " + name + " is not supported",
                            x.base.base.loc);
                    }
                } else {
                    throw SemanticError("Unsupported Decorator type",
                        x.base.base.loc);
                }
            }
        }
        for (size_t i=0; i<x.m_args.n_args; i++) {
            char *arg=x.m_args.m_args[i].m_arg;
            Location loc = x.m_args.m_args[i].loc;
            if (x.m_args.m_args[i].m_annotation == nullptr) {
                throw SemanticError("Argument does not have a type", loc);
            }
            ASR::ttype_t *arg_type = ast_expr_to_asr_type(x.base.base.loc, *x.m_args.m_args[i].m_annotation);
            // Set the function as generic if an argument is typed with a type parameter
            if (ASRUtils::is_generic(*arg_type)) {
                ASR::ttype_t *new_tt = ASRUtils::duplicate_type_without_dims(al, ASRUtils::get_type_parameter(arg_type));
                size_t current_size = tps.size();
                if (current_size == 0) {
                    tps.push_back(al, new_tt);
                } else {
                    bool not_found = true;
                    for (size_t i = 0; i < current_size; i++) {
                        ASR::TypeParameter_t *added_tp = ASR::down_cast<ASR::TypeParameter_t>(tps.p[i]);
                        std::string new_param = ASR::down_cast<ASR::TypeParameter_t>(new_tt)->m_param;
                        std::string added_param = added_tp->m_param;
                        if (added_param.compare(new_param) == 0) {
                            not_found = false;
                            break;
                        }
                    }
                    if (not_found) {
                        tps.push_back(al, new_tt);
                    }
                }
            }

            std::string arg_s = arg;

            ASR::expr_t *value = nullptr;
            ASR::expr_t *init_expr = nullptr;
            ASR::intentType s_intent = ASRUtils::intent_in;
            if (ASRUtils::is_array(arg_type)) {
                s_intent = ASRUtils::intent_inout;
            }
            ASR::storage_typeType storage_type =
                    ASR::storage_typeType::Default;
            ASR::accessType s_access = ASR::accessType::Public;
            ASR::presenceType s_presence = ASR::presenceType::Required;
            bool value_attr = false;
            if (current_procedure_abi_type == ASR::abiType::BindC) {
                value_attr = true;
            }
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                    s2c(al, arg_s), s_intent, init_expr, value, storage_type, arg_type,
                    current_procedure_abi_type, s_access, s_presence,
                    value_attr);
            current_scope->add_symbol(arg_s, ASR::down_cast<ASR::symbol_t>(v));

            ASR::symbol_t *var = current_scope->get_symbol(arg_s);
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
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
            throw SemanticError("Subroutine already defined", tmp->loc);
        }
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        if (current_procedure_abi_type == ASR::abiType::BindC &&
                current_procedure_interface) {
            deftype = ASR::deftypeType::Interface;
        }
        char *bindc_name=nullptr;
        if (x.m_returns && !AST::is_a<AST::ConstantNone_t>(*x.m_returns)) {
            if (AST::is_a<AST::Name_t>(*x.m_returns) || AST::is_a<AST::Subscript_t>(*x.m_returns)) {
                std::string return_var_name = "_lpython_return_variable";
                ASR::ttype_t *type = ast_expr_to_asr_type(x.m_returns->base.loc, *x.m_returns);
                ASR::asr_t *return_var = ASR::make_Variable_t(al, x.m_returns->base.loc,
                    current_scope, s2c(al, return_var_name), ASRUtils::intent_return_var, nullptr, nullptr,
                    ASR::storage_typeType::Default, type,
                    current_procedure_abi_type, ASR::Public, ASR::presenceType::Required,
                    false);
                LFORTRAN_ASSERT(current_scope->get_scope().find(return_var_name) == current_scope->get_scope().end())
                current_scope->add_symbol(return_var_name,
                        ASR::down_cast<ASR::symbol_t>(return_var));
                ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
                    current_scope->get_symbol(return_var_name));
                tmp = ASR::make_Function_t(
                    al, x.base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ s2c(al, sym_name),
                    /* a_args */ args.p,
                    /* n_args */ args.size(),
                    /* a_type_params */ tps.p,
                    /* n_type_params */ tps.size(),
                    /* a_body */ nullptr,
                    /* n_body */ 0,
                    /* a_return_var */ ASRUtils::EXPR(return_var_ref),
                    current_procedure_abi_type,
                    s_access, deftype, bindc_name, vectorize, false, false, is_inline);
            } else {
                throw SemanticError("Return variable must be an identifier (Name AST node) or an array (Subscript AST node)",
                    x.m_returns->base.loc);
            }
        } else {
            bool is_pure = false, is_module = false;
            tmp = ASR::make_Function_t(
                al, x.base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ s2c(al, sym_name),
                /* a_args */ args.p,
                /* n_args */ args.size(),
                /* a_type_params */ tps.p,
                /* n_type_params */ tps.size(),
                /* a_body */ nullptr,
                /* n_body */ 0,
                nullptr,
                current_procedure_abi_type,
                s_access, deftype, bindc_name,
                false, is_pure, is_module, is_inline);
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

    void visit_ImportFrom(const AST::ImportFrom_t &x) {
        if (!x.m_module) {
            throw SemanticError("Not implemented: The import statement must currently specify the module name", x.base.base.loc);
        }
        std::string msym = x.m_module; // Module name
        std::vector<std::string> mod_symbols;
        for (size_t i=0; i<x.n_names; i++) {
            mod_symbols.push_back(x.m_names[i].m_name);
        }

        // Get the module, for now assuming it is not loaded, so we load it:
        ASR::symbol_t *t = nullptr; // current_scope->parent->resolve_symbol(msym);
        if (!t) {
            std::string rl_path = get_runtime_library_dir();
            SymbolTable *st = current_scope;
            std::vector<std::string> paths = {rl_path, parent_dir};
            if (!main_module) {
                st = st->parent;
            }
            bool ltypes;
            t = (ASR::symbol_t*)(load_module(al, st,
                msym, x.base.base.loc, false, paths, ltypes,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); }
                ));
            if (ltypes) {
                // TODO: For now we skip ltypes import completely. Later on we should note what symbols
                // got imported from it, and give an error message if an annotation is used without
                // importing it.
                tmp = nullptr;
                return;
            }
            if (!t) {
                throw SemanticError("The module '" + msym + "' cannot be loaded",
                        x.base.base.loc);
            }
            current_module_dependencies.push_back(al, s2c(al, msym));
        }

        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
        for (auto &remote_sym : mod_symbols) {
            if( procedures_db.is_function_to_be_ignored(msym, remote_sym) ) {
                continue ;
            }
            ASR::symbol_t *t = import_from_module(al, m, current_scope, msym,
                                remote_sym, remote_sym, x.base.base.loc);
            current_scope->add_symbol(remote_sym, t);
        }

        tmp = nullptr;
    }

    void visit_Import(const AST::Import_t &x) {
        ASR::symbol_t *t = nullptr;
        std::string rl_path = get_runtime_library_dir();
        std::vector<std::string> paths = {rl_path, parent_dir};
        SymbolTable *st = current_scope;
        std::vector<std::string> mods;
        for (size_t i=0; i<x.n_names; i++) {
            mods.push_back(x.m_names[i].m_name);
        }
        if (!main_module) {
            st = st->parent;
        }
        for (auto &mod_sym : mods) {
            bool ltypes;
            t = (ASR::symbol_t*)(load_module(al, st,
                mod_sym, x.base.base.loc, false, paths, ltypes,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); }
                ));
            if (ltypes) {
                // TODO: For now we skip ltypes import completely. Later on we should note what symbols
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
                        const char* type_list[14]
                            = { "list", "set", "dict", "tuple",
                                "i8", "i16", "i32", "i64", "f32",
                                "f64", "c32", "c64", "str", "bool"};
                        for (int i = 0; i < 14; i++) {
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

                        Vec<ASR::restriction_t*> restrictions;
                        restrictions.reserve(al, 4);
                        if (rh->n_keywords > 0) {
                            AST::keyword_t keyword = rh->m_keywords[0];
                            if (keyword.m_arg && strcmp(keyword.m_arg, "bound") == 0) {
                                std::set<ASR::traitType> traits;
                                traits = get_traits_from_bounds(keyword.m_value, traits);
                                for (ASR::traitType const trait: traits) {
                                    ASR::restriction_t *restriction = ASR::down_cast<ASR::restriction_t>(
                                        ASR::make_Restriction_t(al, keyword.loc, trait));
                                    restrictions.push_back(al, restriction);
                                }
                            }
                        }

                        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_TypeParameter_t(al, x.base.base.loc, s2c(al, tvar_name),
                            dims.p, dims.size(), restrictions.p, restrictions.size()));

                        ASR::expr_t *value = nullptr;
                        ASR::expr_t *init_expr = nullptr;
                        ASR::intentType s_intent = ASRUtils::intent_local;
                        ASR::storage_typeType storage_type = ASR::storage_typeType::Default;
                        ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
                        ASR::accessType s_access = ASR::accessType::Public;
                        ASR::presenceType s_presence = ASR::presenceType::Required;
                        bool value_attr = false;

                        // Build the variable and add it to the scope
                        ASR::asr_t *v = ASR::make_Variable_t(al, x.base.base.loc, current_scope,
                            s2c(al, tvar_name), s_intent, init_expr, value, storage_type, type,
                            current_procedure_abi_type, s_access, s_presence,
                            value_attr);
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

    /**
     *  Convert bounds in type variables declarations into restrictions
     *    T = TypeVar('T', bounds=...(|...)*)
     */
    std::set<ASR::traitType> get_traits_from_bounds(AST::expr_t *value, std::set<ASR::traitType> traits) {
        if (AST::is_a<AST::Name_t>(*value)) {
            std::string trait_name = AST::down_cast<AST::Name_t>(value)->m_id;
            if (trait_name == "Any") {
                traits.insert(ASR::traitType::Any);
            } else if (trait_name == "SupportsPlus") {
                traits.insert(ASR::traitType::SupportsPlus);
            } else if (trait_name == "SupportsZero") {
                traits.insert(ASR::traitType::SupportsZero);
            } else if (trait_name == "Divisible") {
                traits.insert(ASR::traitType::Divisible);
            } else {
                throw SemanticError("Unsupported trait " + trait_name, value->base.loc);
            }
        } else if (AST::is_a<AST::BinOp_t>(*value)) {
            AST::BinOp_t *binop = AST::down_cast<AST::BinOp_t>(value);
            if (binop->m_op == AST::operatorType::BitOr) {
                traits = get_traits_from_bounds(binop->m_left, traits);
                traits = get_traits_from_bounds(binop->m_right, traits);
            }
        } else {
            throw SemanticError("Unsupported expression for restrictions", value->base.loc);
        }
        return traits;
    }

    void visit_Expr(const AST::Expr_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
};

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, const AST::Module_t &ast,
        diag::Diagnostics &diagnostics, bool main_module,
        std::map<int, ASR::symbol_t*> &ast_overload, std::string parent_dir)
{
    SymbolTableVisitor v(al, nullptr, diagnostics, main_module, ast_overload, parent_dir);
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


    BodyVisitor(Allocator &al, ASR::asr_t *unit, diag::Diagnostics &diagnostics,
         bool main_module, std::map<int, ASR::symbol_t*> &ast_overload)
         : CommonVisitor(al, nullptr, diagnostics, main_module, ast_overload, ""), asr{unit}
         {}

    // Transforms statements to a list of ASR statements
    // In addition, it also inserts the following nodes if needed:
    //   * ImplicitDeallocate
    //   * GoToTarget
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
                ASR::stmt_t* tmp_stmt = ASRUtils::STMT(tmp);
                body.push_back(al, tmp_stmt);
            } else if (!tmp_vec.empty()) {
                for (auto t: tmp_vec) {
                    if (t != nullptr) {
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
        current_scope = unit->m_global_scope;
        LFORTRAN_ASSERT(current_scope != nullptr);
        if (!main_module) {
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(current_scope->get_symbol("__main__"));
            current_scope = mod->m_symtab;
            LFORTRAN_ASSERT(current_scope != nullptr);
        }

        Vec<ASR::asr_t*> items;
        items.reserve(al, 4);
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
        // These global statements are added to the translation unit for now,
        // but they should be adding to a module initialization function
        unit->m_items = items.p;
        unit->n_items = items.size();

        tmp = asr;
    }

    void handle_fn(const AST::FunctionDef_t &x, ASR::Function_t &v) {
        current_scope = v.m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        v.m_body = body.p;
        v.n_body = body.size();
    }

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->get_symbol(x.m_name);
        if (ASR::is_a<ASR::Function_t>(*t)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(t);
            handle_fn(x, *f);
        } else if (ASR::is_a<ASR::GenericProcedure_t>(*t)) {
            ASR::symbol_t *s = ast_overload[(int64_t)&x];
            if (ASR::is_a<ASR::Function_t>(*s)) {
                ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(s);
                handle_fn(x, *f);
            } else {
                LFORTRAN_ASSERT(false);
            }
        } else {
            LFORTRAN_ASSERT(false);
        }
        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Import(const AST::Import_t &/*x*/) {
        // visited in symbol visitor
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
            if (current_scope->parent != nullptr) {
                // Re-declaring a global scope variable is allowed,
                // otherwise raise an error
                ASR::symbol_t *orig_decl = current_scope->get_symbol(var_name);
                throw SemanticError(diag::Diagnostic(
                    "Symbol is already declared in the same scope",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("original declaration", {orig_decl->base.loc}, false),
                        diag::Label("redeclaration", {x.base.base.loc}),
                    }));
            }
        }

        visit_AnnAssignUtil(x, var_name);
    }

    void visit_Delete(const AST::Delete_t &x) {
        if (x.n_targets == 0) {
            throw SemanticError("Delete statement must be operated on at least one target",
                    x.base.base.loc);
        }
        Vec<ASR::symbol_t*> targets;
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
                targets.push_back(al, s);
            } else {
                throw SemanticError("Only Name supported for now as target of Delete",
                        x.base.base.loc);
            }
        }
        tmp = ASR::make_ExplicitDeallocate_t(al, x.base.base.loc, targets.p,
                targets.size());
    }

    void visit_Assign(const AST::Assign_t &x) {
        ASR::expr_t *target, *assign_value = nullptr, *tmp_value;
        this->visit_expr(*x.m_value);
        if (tmp) {
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
            if (AST::is_a<AST::Subscript_t>(*x.m_targets[i])) {
                AST::Subscript_t *sb = AST::down_cast<AST::Subscript_t>(x.m_targets[i]);
                if (AST::is_a<AST::Name_t>(*sb->m_value)) {
                    std::string name = AST::down_cast<AST::Name_t>(sb->m_value)->m_id;
                    ASR::symbol_t *s = current_scope->resolve_symbol(name);
                    if (!s) {
                        throw SemanticError("Variable: '" + name + "' is not declared",
                                x.base.base.loc);
                    }
                    ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
                    ASR::ttype_t *type = v->m_type;
                    if (ASR::is_a<ASR::Dict_t>(*type)) {
                        // dict insert case;
                        this->visit_expr(*sb->m_slice);
                        ASR::expr_t *key = ASRUtils::EXPR(tmp);
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
                        ASR::expr_t* se = ASR::down_cast<ASR::expr_t>(
                                ASR::make_Var_t(al, x.base.base.loc, s));
                        tmp = nullptr;
                        tmp_vec.push_back(make_DictInsert_t(al, x.base.base.loc, se, key, tmp_value));
                        continue;
                    } else if (ASRUtils::is_immutable(type)) {
                        throw SemanticError("'" + ASRUtils::type_to_str_python(type) + "' object does not support"
                            " item assignment", x.base.base.loc);
                    }
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
            if (!tmp_value) continue;
            this->visit_expr(*x.m_targets[i]);
            target = ASRUtils::EXPR(tmp);
            ASR::ttype_t *target_type = ASRUtils::expr_type(target);
            ASR::ttype_t *value_type = ASRUtils::expr_type(assign_value);
            // Check if the target parameter type can be assigned with zero
            if (ASR::is_a<ASR::TypeParameter_t>(*target_type)
                    && ASR::is_a<ASR::IntegerConstant_t>(*assign_value)) {
                ASR::IntegerConstant_t *value_constant = ASR::down_cast<ASR::IntegerConstant_t>(assign_value);
                if (value_constant->m_n == 0) {
                    if (!ASRUtils::has_trait(ASR::down_cast<ASR::TypeParameter_t>(target_type),
                                             ASR::traitType::SupportsZero)) {
                        throw SemanticError("A generic variable must support zero "
                                            "to be assignable with zero.",
                                            target_type->base.loc);
                    }
                    tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, assign_value, nullptr);
                    return ;
                }
            }
            if( ASR::is_a<ASR::Pointer_t>(*target_type) &&
                ASR::is_a<ASR::Var_t>(*target) ) {
                if( !ASR::is_a<ASR::GetPointer_t>(*tmp_value) ) {
                    throw SemanticError("A pointer variable can only "
                                        "be associated with the output "
                                        "of pointer() call.",
                                        tmp_value->base.loc);
                }
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
            if( ann_assign_target_type == nullptr ) {
                tmp = nullptr;
                return ;
            }
            type = ASRUtils::get_contained_type(ann_assign_target_type);
        }
        ASR::ttype_t* list_type = ASRUtils::TYPE(ASR::make_List_t(al, x.base.base.loc, type));
        tmp = ASR::make_ListConstant_t(al, x.base.base.loc, list.p,
            list.size(), list_type);
    }

    ASR::expr_t* for_iterable_helper(std::string var_name, const Location& loc) {
        auto loop_src_var_symbol = current_scope->resolve_symbol(var_name);
        LFORTRAN_ASSERT(loop_src_var_symbol!=nullptr);
        auto loop_src_var_ttype = ASRUtils::symbol_type(loop_src_var_symbol);
        auto int_type = ASR::make_Integer_t(al, loc, 4, nullptr, 0);
        // create a new variable called/named __explicit_iterator of type i32 and add it to symbol table
        std::string explicit_iter_name = current_scope->get_unique_name("__explicit_iterator");
        auto explicit_iter_variable = ASR::make_Variable_t(al, loc,
            current_scope, s2c(al, explicit_iter_name), ASR::intentType::Local,
            nullptr, nullptr, ASR::storage_typeType::Default,
            ASRUtils::TYPE(int_type), ASR::abiType::Source,
            ASR::accessType::Public, ASR::presenceType::Required, false
        );

        current_scope->add_symbol(explicit_iter_name,
                        ASR::down_cast<ASR::symbol_t>(explicit_iter_variable));
        // make loop_end = len(loop_src_var), where loop_src_var is the variable over which
        // we are iterating the for in loop
        auto loop_src_var = ASR::make_Var_t(al, loc, loop_src_var_symbol);
        if (ASR::is_a<ASR::Character_t>(*loop_src_var_ttype)) {
            return ASRUtils::EXPR(ASR::make_StringLen_t(al, loc,
                                  ASRUtils::EXPR(loop_src_var),
                                  ASRUtils::TYPE(int_type), nullptr));
        } else if (ASR::is_a<ASR::List_t>(*loop_src_var_ttype)) {
            return ASRUtils::EXPR(ASR::make_ListLen_t(al, loc,
                                  ASRUtils::EXPR(loop_src_var),
                                  ASRUtils::TYPE(int_type), nullptr));
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

    void visit_For(const AST::For_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target=ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        bool is_explicit_iterator_required = false;
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
                throw SemanticError("Only range(..) supported as for loop iteration for now",
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
            loop_end = for_iterable_helper(loop_src_var_name, x.base.base.loc);
            for_iter_type = loop_end;
            LFORTRAN_ASSERT(loop_end);
            is_explicit_iterator_required = true;
        } else if (AST::is_a<AST::Subscript_t>(*x.m_iter)) {
            AST::Subscript_t *sbt = AST::down_cast<AST::Subscript_t>(x.m_iter);
            if (AST::is_a<AST::Name_t>(*sbt->m_value)) {
                loop_src_var_name = AST::down_cast<AST::Name_t>(sbt->m_value)->m_id;
                visit_Subscript(*sbt);
                ASR::expr_t *target = ASRUtils::EXPR(tmp);
                auto loop_src_var_symbol = current_scope->resolve_symbol(loop_src_var_name);
                auto loop_src_var_ttype = ASRUtils::symbol_type(loop_src_var_symbol);
                std::string tmp_assign_name = current_scope->get_unique_name("__tmp_assign_for_loop");
                auto tmp_assign_variable = ASR::make_Variable_t(al, sbt->base.base.loc, current_scope,
                    s2c(al, tmp_assign_name), ASR::intentType::Local, nullptr, target, ASR::storage_typeType::Default,
                    loop_src_var_ttype, ASR::abiType::Source, ASR::accessType::Public, ASR::presenceType::Required, false
                );
                current_scope->add_symbol(tmp_assign_name, ASR::down_cast<ASR::symbol_t>(tmp_assign_variable));
                loop_end = for_iterable_helper(tmp_assign_name, x.base.base.loc);
                for_iter_type = loop_end;
                LFORTRAN_ASSERT(loop_end);
                loop_src_var_name = tmp_assign_name;
                is_explicit_iterator_required = true;
            } else {
                throw SemanticError("Only Name is supported for Subscript",
                    sbt->base.base.loc);
            }
        } else {
            throw SemanticError("Only function call `range(..)` supported as for loop iteration for now",
                x.base.base.loc);
        }
        int a_kind = 4;
        if (!is_explicit_iterator_required) {
            a_kind = ASRUtils::extract_kind_from_ttype_t(ASRUtils::expr_type(target));
        }
        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            a_kind, nullptr, 0));
        ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(
                                            al, x.base.base.loc, 1, a_type));
        make_BinOp_helper(loop_end, constant_one, ASR::binopType::Sub,
                            x.base.base.loc, false);
        loop_end = ASRUtils::EXPR(tmp);
        ASR::do_loop_head_t head;

        if(is_explicit_iterator_required) {
            body.reserve(al, x.n_body + 1);
            // add an assignment instruction to body to assign value of loop_src_var at an index to the loop_target_var
            auto explicit_iter_var = ASR::make_Var_t(al, x.base.base.loc, current_scope->get_symbol("__explicit_iterator"));
            auto index_plus_one = ASR::make_IntegerBinOp_t(al, x.base.base.loc, ASRUtils::EXPR(explicit_iter_var),
                ASR::binopType::Add, constant_one, a_type, nullptr);
            auto loop_src_var = ASR::make_Var_t(al, x.base.base.loc, current_scope->resolve_symbol(loop_src_var_name));
            ASR::asr_t* loop_src_var_element = nullptr;
            if (ASR::is_a<ASR::StringLen_t>(*for_iter_type)) {
                loop_src_var_element = ASR::make_StringItem_t(
                            al, x.base.base.loc, ASRUtils::EXPR(loop_src_var),
                            ASRUtils::EXPR(index_plus_one), a_type, nullptr);
            } else if (ASR::is_a<ASR::ListLen_t>(*for_iter_type)) {
                loop_src_var_element = ASR::make_ListItem_t(
                            al, x.base.base.loc, ASRUtils::EXPR(loop_src_var),
                            ASRUtils::EXPR(explicit_iter_var), a_type, nullptr);
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
        transform_stmts(body, x.n_body, x.m_body);
        int32_t total_syms = current_scope->get_scope().size();
        for( auto& item: current_scope->get_scope() ) {
            total_syms -= ASR::is_a<ASR::ExternalSymbol_t>(*item.second);
        }
        if( total_syms > 0 ) {
            std::string name = parent_scope->get_unique_name("block");
            ASR::asr_t* block = ASR::make_Block_t(al, x.base.base.loc,
                                                current_scope, s2c(al, name),
                                                body.p, body.size());
            current_scope = parent_scope;
            current_scope->add_symbol(name, ASR::down_cast<ASR::symbol_t>(block));
            ASR::stmt_t* decls = ASRUtils::STMT(ASR::make_BlockCall_t(al, x.base.base.loc,  -1,
                ASR::down_cast<ASR::symbol_t>(block)));
            body.reserve(al, 1);
            body.push_back(al, decls);
        } else {
            // Revert global counter as no variables
            // were declared inside the loop so
            // current_scope is not needed.
            for( auto& item: current_scope->get_scope() ) {
                if( !ASR::is_a<ASR::ExternalSymbol_t>(*item.second) ) {
                    continue ;
                }

                ASR::ExternalSymbol_t* ext_sym = ASR::down_cast<ASR::ExternalSymbol_t>(item.second);
                ASR::symbol_t* new_ext_sym = ASR::down_cast<ASR::symbol_t>(
                    ASR::make_ExternalSymbol_t(al, ext_sym->base.base.loc, parent_scope, ext_sym->m_name,
                            ext_sym->m_external, ext_sym->m_module_name, ext_sym->m_scope_names,
                            ext_sym->n_scope_names, ext_sym->m_original_name, ext_sym->m_access));
                parent_scope->add_symbol(item.first, new_ext_sym);
            }
            current_scope = parent_scope;
        }

        if (loop_start) {
            head.m_start = loop_start;
        } else {
            head.m_start = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, x.base.base.loc, 0, a_type));
        }
        head.m_end = loop_end;
        if (inc) {
            head.m_increment = inc;
        } else {
            head.m_increment = ASR::down_cast<ASR::expr_t>(ASR::make_IntegerConstant_t(al, x.base.base.loc, 1, a_type));
        }
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
            tmp = ASR::make_DoLoop_t(al, x.base.base.loc, head,
                body.p, body.size());
        }
    }

    void visit_AugAssign(const AST::AugAssign_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);
        ASR::ttype_t* left_type = ASRUtils::expr_type(left);
        ASR::ttype_t* right_type = ASRUtils::expr_type(right);
        ASR::binopType op;
        switch (x.m_op) {
            case (AST::operatorType::Add) : { op = ASR::binopType::Add; break; }
            case (AST::operatorType::Sub) : { op = ASR::binopType::Sub; break; }
            case (AST::operatorType::Mult) : { op = ASR::binopType::Mul; break; }
            case (AST::operatorType::Div) : { op = ASR::binopType::Div; break; }
            case (AST::operatorType::Pow) : { op = ASR::binopType::Pow; break; }
            default : {
                throw SemanticError("Binary operator type not supported",
                    x.base.base.loc);
            }
        }

        make_BinOp_helper(left, right, op, x.base.base.loc, false);
        if( !ASRUtils::check_equal_type(left_type, right_type) ) {
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
        ASR::stmt_t* a_overloaded = nullptr;
        ASR::expr_t *tmp2 = ASR::down_cast<ASR::expr_t>(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, a_overloaded);

    }

    void visit_AttributeUtil(ASR::ttype_t* type, char* attr_char,
                             ASR::expr_t *e, const Location& loc) {
        if( ASR::is_a<ASR::Derived_t>(*type) ) {
            ASR::Derived_t* der = ASR::down_cast<ASR::Derived_t>(type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_derived_type);
            ASR::DerivedType_t* der_type = ASR::down_cast<ASR::DerivedType_t>(der_sym);
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
            LFORTRAN_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            tmp = ASR::make_DerivedRef_t(al, loc, e, member_sym,
                                         member_var->m_type, nullptr);
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
        if (ASRUtils::is_complex(*type)) {
            std::string attr = attr_char;
            if (attr == "imag") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
                int kind = ASRUtils::extract_kind_from_ttype_t(type);
                ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc,
                                                kind, nullptr, 0));
                tmp = ASR::make_ComplexIm_t(al, loc, val, dest_type, nullptr);
                return;
            } else if (attr == "real") {
                ASR::expr_t *val = ASR::down_cast<ASR::expr_t>(ASR::make_Var_t(al, loc, t));
                int kind = ASRUtils::extract_kind_from_ttype_t(type);
                ASR::ttype_t *dest_type = ASR::down_cast<ASR::ttype_t>(ASR::make_Real_t(al, loc,
                                                kind, nullptr, 0));
                ASR::expr_t *value = ASR::down_cast<ASR::expr_t>(ASRUtils::make_Cast_t_value(
                    al, val->base.loc, val, ASR::cast_kindType::ComplexToReal, dest_type));
                tmp = ASR::make_ComplexRe_t(al, loc, val, dest_type, ASRUtils::expr_value(value));
                return;
            } else {
                throw SemanticError("'" + attr + "' is not implemented for Complex type",
                    loc);
            }
        } else if( ASR::is_a<ASR::Derived_t>(*type) ) {
            ASR::Derived_t* der = ASR::down_cast<ASR::Derived_t>(type);
            ASR::symbol_t* der_sym = ASRUtils::symbol_get_past_external(der->m_derived_type);
            ASR::DerivedType_t* der_type = ASR::down_cast<ASR::DerivedType_t>(der_sym);
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
            LFORTRAN_ASSERT(ASR::is_a<ASR::Variable_t>(*member_sym));
            ASR::Variable_t* member_var = ASR::down_cast<ASR::Variable_t>(member_sym);
            tmp = ASR::make_DerivedRef_t(al, loc, val, member_sym,
                                            member_var->m_type, nullptr);
        } else if(ASR::is_a<ASR::Pointer_t>(*type)) {
            ASR::Pointer_t* p = ASR::down_cast<ASR::Pointer_t>(type);
            visit_AttributeUtil(p->m_type, attr_char, t, loc);
        } else {
            throw SemanticError(ASRUtils::type_to_str_python(type) + " not supported yet in Attribute.",
                loc);
        }
    }

    void visit_Attribute(const AST::Attribute_t &x) {
        if (AST::is_a<AST::Name_t>(*x.m_value)) {
            std::string value = AST::down_cast<AST::Name_t>(x.m_value)->m_id;
            ASR::symbol_t *t = current_scope->resolve_symbol(value);
            if (!t) {
                throw SemanticError("'" + value + "' is not defined in the scope",
                    x.base.base.loc);
            }
            if (ASR::is_a<ASR::Variable_t>(*t)) {
                ASR::Variable_t *var = ASR::down_cast<ASR::Variable_t>(t);
                visit_AttributeUtil(var->m_type, x.m_attr, t, x.base.base.loc);
            } else {
                throw SemanticError("Only Variable type is supported for now in Attribute",
                    x.base.base.loc);
            }

        } else if(AST::is_a<AST::Attribute_t>(*x.m_value)) {
            AST::Attribute_t* x_m_value = AST::down_cast<AST::Attribute_t>(x.m_value);
            visit_Attribute(*x_m_value);
            ASR::expr_t* e = ASRUtils::EXPR(tmp);
            visit_AttributeUtil(ASRUtils::expr_type(e), x.m_attr, e, x.base.base.loc);
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
        LFORTRAN_ASSERT(x.n_keys == x.n_values);
        if( x.n_keys == 0 && ann_assign_target_type != nullptr ) {
            tmp = ASR::make_DictConstant_t(al, x.base.base.loc, nullptr, 0,
                                           nullptr, 0, ann_assign_target_type);
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
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, test, body.p,
                body.size());
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
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
        ASR::expr_t *overloaded = nullptr;
        if (((left_type->type != ASR::ttypeType::Real &&
            left_type->type != ASR::ttypeType::Integer) &&
            (right_type->type != ASR::ttypeType::Real &&
            right_type->type != ASR::ttypeType::Integer) &&
            ((left_type->type != ASR::ttypeType::Complex ||
            right_type->type != ASR::ttypeType::Complex) &&
            x.m_ops != AST::cmpopType::Eq && x.m_ops != AST::cmpopType::NotEq) &&
            (left_type->type != ASR::ttypeType::Character ||
            right_type->type != ASR::ttypeType::Character)) &&
            (left_type->type != ASR::ttypeType::Logical ||
            right_type->type != ASR::ttypeType::Logical)) {
        throw SemanticError(
            "Compare: only Integer, Real, Logical, or String can be on the LHS and RHS."
            "If operator is Eq or NotEq then Complex type is also acceptable",
            x.base.base.loc);
        }

        if (!ASRUtils::is_logical(*left_type) || !ASRUtils::is_logical(*right_type)) {
            cast_helper(left, right, false);
        }
        ASR::ttype_t *dest_type = ASRUtils::expr_type(left);

        // Check that the types are now the same
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(left),
                                    ASRUtils::expr_type(right))) {
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
            ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::expr_t *value = nullptr;

        if (ASRUtils::is_integer(*dest_type)) {

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                int64_t left_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::IntegerConstant_t>(
                                        ASRUtils::expr_value(right))->m_n;
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
                    default: LFORTRAN_ASSERT(false); // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_LogicalConstant_t(
                    al, x.base.base.loc, result, type));
            }

            tmp = ASR::make_StringCompare_t(al, x.base.base.loc, left, asr_op, right, type, value);
        }

        if (overloaded != nullptr) {
            tmp = ASR::make_OverloadedCompare_t(al, x.base.base.loc, left, asr_op, right, type,
                value, overloaded);
        }
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
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc, return_var);
        ASR::expr_t *target = ASRUtils::EXPR(return_var_ref);
        ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
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

        // We can only return one statement in `tmp`, so we insert the current
        // `tmp` into the body of the function directly
        current_body->push_back(al, ASR::down_cast<ASR::stmt_t>(tmp));

        // Now we assign Return into `tmp`
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    void visit_Continue(const AST::Continue_t &x) {
        tmp = ASR::make_Cycle_t(al, x.base.base.loc);
    }

    void visit_Break(const AST::Break_t &x) {
        tmp = ASR::make_Exit_t(al, x.base.base.loc);
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
        LFORTRAN_ASSERT(x.n_elts > 0); // type({}) is 'dict'
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
            std::string call_name;
            if (AST::is_a<AST::Name_t>(*c->m_func)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(c->m_func);
                call_name = n->m_id;
                ASR::symbol_t* s = current_scope->resolve_symbol(call_name);
                if( call_name == "c_p_pointer" && !s ) {
                    tmp = create_CPtrToPointer(*c);
                    return;
                }
                if( call_name == "p_c_pointer" && !s ) {
                    tmp = create_PointerToCPtr(*c);
                    return;
                }
            } else if (AST::is_a<AST::Attribute_t>(*c->m_func)) {
                AST::Attribute_t *at = AST::down_cast<AST::Attribute_t>(c->m_func);
                if (AST::is_a<AST::Name_t>(*at->m_value)) {
                    std::string value = AST::down_cast<AST::Name_t>(at->m_value)->m_id;
                    ASR::symbol_t *t = current_scope->resolve_symbol(value);
                    if (!t) {
                        throw SemanticError("'" + value + "' is not defined in the scope",
                            x.base.base.loc);
                    }
                    Vec<ASR::expr_t*> elements;
                    elements.reserve(al, c->n_args);
                    for (size_t i = 0; i < c->n_args; ++i) {
                        visit_expr(*c->m_args[i]);
                        elements.push_back(al, ASRUtils::EXPR(tmp));
                    }
                    ASR::expr_t *te = ASR::down_cast<ASR::expr_t>(
                                        ASR::make_Var_t(al, x.base.base.loc, t));
                    handle_attribute(te, at->m_attr, x.base.base.loc, elements);
                    return;
                }
            } else {
                throw SemanticError("Only Name/Attribute supported in Call",
                    x.base.base.loc);
            }

            Vec<ASR::call_arg_t> args;
            // Keyword arguments to be handled in make_call_helper
            args.reserve(al, c->n_args);
            visit_expr_list(c->m_args, c->n_args, args);
            if (call_name == "print") {
                ASR::expr_t *fmt = nullptr;
                Vec<ASR::expr_t*> args_expr = ASRUtils::call_arg2expr(al, args);
                ASR::expr_t *separator = nullptr;
                ASR::expr_t *end = nullptr;
                if (c->n_keywords > 0) {
                    std::string arg_name;
                    for (size_t i = 0; i < c->n_keywords; i++) {
                        arg_name = c->m_keywords[i].m_arg;
                        if (arg_name == "sep") {
                            visit_expr(*c->m_keywords[i].m_value);
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
                            visit_expr(*c->m_keywords[i].m_value);
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
                tmp = ASR::make_Print_t(al, x.base.base.loc, fmt,
                    args_expr.p, args_expr.size(), separator, end);
                return;

            } else if (call_name == "quit") {
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
            }
            ASR::symbol_t *s = current_scope->resolve_symbol(call_name);
            if (!s) {
                throw SemanticError("Function '" + call_name + "' is not declared",
                    x.base.base.loc);
            }
            tmp = make_call_helper(al, s, current_scope, args, call_name,
                    x.base.base.loc, true, c->m_args, c->n_args, c->m_keywords,
                    c->n_keywords);
            return;
        }
        this->visit_expr(*x.m_value);
        ASRUtils::EXPR(tmp);
        tmp = nullptr;
    }


    ASR::asr_t* create_CPtrToPointer(const AST::Call_t& x) {
        if( x.n_args != 2 ) {
            throw SemanticError("c_p_pointer accepts two positional arguments, "
                                "first a variable of c_ptr type and second "
                                " the target type of the first variable.",
                                x.base.base.loc);
        }
        visit_expr(*x.m_args[0]);
        ASR::expr_t* cptr = ASRUtils::EXPR(tmp);
        visit_expr(*x.m_args[1]);
        ASR::expr_t* pptr = ASRUtils::EXPR(tmp);
        return ASR::make_CPtrToPointer_t(al, x.base.base.loc, cptr,
                                         pptr, nullptr);
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

    void visit_Call(const AST::Call_t &x) {
        std::string call_name;
        Vec<ASR::call_arg_t> args;
        // Keyword arguments handled in make_call_helper
        if( x.n_keywords == 0 ) {
            args.reserve(al, x.n_args);
            visit_expr_list(x.m_args, x.n_args, args);
        }

        if (AST::is_a<AST::Name_t>(*x.m_func)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(x.m_func);
            call_name = n->m_id;
        } else if (AST::is_a<AST::Attribute_t>(*x.m_func)) {
            AST::Attribute_t *at = AST::down_cast<AST::Attribute_t>(x.m_func);
            if (AST::is_a<AST::Name_t>(*at->m_value)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(at->m_value);
                std::string mod_name = n->m_id;
                call_name = at->m_attr;
                std::string call_name_store = "__" + mod_name + "_" + call_name;
                ASR::symbol_t *st = nullptr;
                if (current_scope->resolve_symbol(call_name_store) != nullptr) {
                    st = current_scope->get_symbol(call_name_store);
                } else {
                    SymbolTable *symtab = current_scope;
                    while (symtab->parent != nullptr) symtab = symtab->parent;
                    if (symtab->resolve_symbol(mod_name) == nullptr) {
                        if (current_scope->resolve_symbol(mod_name) != nullptr) {
                            // this case when we have variable and attribute
                            st = current_scope->resolve_symbol(mod_name);
                            Vec<ASR::expr_t*> eles;
                            eles.reserve(al, x.n_args);
                            for (size_t i=0; i<x.n_args; i++) {
                                eles.push_back(al, args[i].m_value);
                            }
                            ASR::expr_t *se = ASR::down_cast<ASR::expr_t>(
                                            ASR::make_Var_t(al, x.base.base.loc, st));
                            if (ASR::is_a<ASR::Character_t>(*(ASRUtils::expr_type(se)))) {
                                if (std::string(at->m_attr) == std::string("capitalize")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.capitalize() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_capitalize");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_capitalize", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("lower")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.lower() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_lower");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_lower", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("find")) {
                                    if(args.size() != 1) {
                                        throw SemanticError("str.find() takes one argument",
                                            x.base.base.loc);
                                    }
                                    ASR::expr_t *arg_sub = args[0].m_value;
                                    ASR::ttype_t *arg_sub_type = ASRUtils::expr_type(arg_sub);
                                    if(!ASRUtils::is_character(*arg_sub_type)) {
                                        throw SemanticError("str.find() takes one argument of type: str",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_find");
                                    Vec<ASR::call_arg_t> function_args;
                                    function_args.reserve(al, 1);
                                    ASR::call_arg_t str;
                                    str.loc = x.base.base.loc;
                                    str.m_value = se;
                                    ASR::call_arg_t sub;
                                    sub.loc = x.base.base.loc;
                                    sub.m_value = args[0].m_value;
                                    function_args.push_back(al, str);
                                    function_args.push_back(al, sub);
                                    tmp = make_call_helper(al, fn_div, current_scope, function_args, "_lpython_str_find", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("rstrip")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.srtrip() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_rstrip");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_rstrip", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("lstrip")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.lrtrip() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_lstrip");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_lstrip", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("strip")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.srtrip() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_strip");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_strip", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("swapcase")) {
                                    if(args.size() != 0) {
                                        throw SemanticError("str.swapcase() takes no arguments",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_swapcase");
                                    Vec<ASR::call_arg_t> args;
                                    args.reserve(al, 1);
                                    ASR::call_arg_t arg;
                                    arg.loc = x.base.base.loc;
                                    arg.m_value = se;
                                    args.push_back(al, arg);
                                    tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_swapcase", x.base.base.loc);
                                    return;
                                } else if (std::string(at->m_attr) == std::string("startswith")) {
                                    if(args.size() != 1) {
                                        throw SemanticError("str.startwith() takes one argument",
                                            x.base.base.loc);
                                    }
                                    ASR::expr_t *arg_sub = args[0].m_value;
                                    ASR::ttype_t *arg_sub_type = ASRUtils::expr_type(arg_sub);
                                    if(!ASRUtils::is_character(*arg_sub_type)) {
                                        throw SemanticError("str.startwith() takes one argument of type: str",
                                            x.base.base.loc);
                                    }
                                    ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_startswith");
                                    Vec<ASR::call_arg_t> function_args;
                                    function_args.reserve(al, 1);
                                    ASR::call_arg_t str;
                                    str.loc = x.base.base.loc;
                                    str.m_value = se;
                                    ASR::call_arg_t sub;
                                    sub.loc = x.base.base.loc;
                                    sub.m_value = args[0].m_value;
                                    function_args.push_back(al, str);
                                    function_args.push_back(al, sub);
                                    tmp = make_call_helper(al, fn_div, current_scope, function_args, "_lpython_str_startswith", x.base.base.loc);
                                    return;
                                }
                            }
                            handle_attribute(se, at->m_attr, x.base.base.loc, eles);
                            return;
                        }
                        throw SemanticError("module '" + mod_name + "' is not imported",
                            x.base.base.loc);
                    }
                    ASR::symbol_t *mt = symtab->get_symbol(mod_name);
                    ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(mt);
                    st = import_from_module(al, m, current_scope, mod_name,
                                        call_name, call_name_store, x.base.base.loc);
                    current_scope->add_symbol(call_name_store, st);
                }
                tmp = make_call_helper(al, st, current_scope, args, call_name, x.base.base.loc);
                return;
            } else if (AST::is_a<AST::ConstantInt_t>(*at->m_value)) {
                if (std::string(at->m_attr) == std::string("bit_length")) {
                    //bit_length() attribute:
                    if(args.size() != 0) {
                        throw SemanticError("int.bit_length() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantInt_t *n = AST::down_cast<AST::ConstantInt_t>(at->m_value);
                    int64_t int_val = std::abs(n->m_value);
                    int32_t res = 0;
                    for(; int_val; int_val >>= 1, res++);
                    ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                        4, nullptr, 0));
                    tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, res, int_type);
                    return;
                } else {
                    throw SemanticError("'int' object has no attribute '" + std::string(at->m_attr) + "'",
                        x.base.base.loc);
                }
            } else if (AST::is_a<AST::ConstantStr_t>(*at->m_value)) {
                if (std::string(at->m_attr) == std::string("capitalize")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.capitalize() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    res[0] = toupper(res[0]);
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("lower")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.lower() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    for (auto &i : res) {
                        if (i >= 'A' && i<= 'Z') {
                            i = tolower(i);
                        }
                    }
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("find")) {
                    if (args.size() != 1) {
                        throw SemanticError("str.find() takes one arguments",
                            x.base.base.loc);
                    }
                    ASR::expr_t *arg = args[0].m_value;
                    ASR::ttype_t *type = ASRUtils::expr_type(arg);
                    if (ASRUtils::is_character(*type)) {
                        AST::ConstantStr_t* str_str_con = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                        std::string str = str_str_con->m_value;
                        if (ASRUtils::expr_value(arg) != nullptr) {
                            ASR::StringConstant_t* sub_str_con = ASR::down_cast<ASR::StringConstant_t>(arg);
                            std::string sub = sub_str_con->m_s;
                            //KMP matching
                            int str_len = str.size();
                            int sub_len = sub.size();
                            bool flag = 0;
                            int res = -1;
                            std::vector<int>lps(sub_len, 0);
                            if (str_len == 0 || sub_len == 0) {
                                res = (!sub_len || (sub_len == str_len))? 0: -1;
                            } else {
                                for(int i = 1, len = 0; i < sub_len;) {
                                    if (sub[i] == sub[len]) {
                                        lps[i++] = ++len;
                                    } else {
                                        if (len != 0) {
                                            len = lps[len - 1];
                                        } else {
                                            lps[i++] = 0;
                                        }
                                    }
                                }
                                for (int i = 0, j = 0; (str_len - i) >= (sub_len - j) && !flag;) {
                                    if (sub[j] == str[i]) {
                                        j++, i++;
                                    }
                                    if (j == sub_len) {
                                        res = i - j;
                                        flag = 1;
                                        j = lps[j - 1];
                                    } else if (i < str_len && sub[j] != str[i]) {
                                        if (j != 0) {
                                            j = lps[j - 1];
                                        } else {
                                            i = i + 1;
                                        }
                                    }
                                }
                            }
                            tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, res, ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                                        4, nullptr, 0)));
                        } else {
                            ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_find");
                            Vec<ASR::call_arg_t> args;
                            args.reserve(al, 1);
                            ASR::call_arg_t str_arg;
                            str_arg.loc = x.base.base.loc;
                            ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 0, nullptr, nullptr, 0));
                            str_arg.m_value = ASRUtils::EXPR(
                                    ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, str), str_type));
                            ASR::call_arg_t sub_arg;
                            sub_arg.loc = x.base.base.loc;
                            sub_arg.m_value = arg;
                            args.push_back(al, str_arg);
                            args.push_back(al, sub_arg);
                            tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_find", x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("str.find() takes one arguments of type: str",
                            arg->base.loc);
                    }
                    return;
                } else if (std::string(at->m_attr) == std::string("rstrip")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.rstrip() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    int ind = (int)res.size() - 1;
                    while (ind >= 0 && res[ind] == ' '){
                        ind--;
                    }
                    res = std::string(res.begin(), res.begin() + ind +1);
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("lstrip")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.lstrip() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    size_t ind = 0;
                    while (ind < res.size() && res[ind] == ' ') {
                        ind++;
                    }
                    res = std::string(res.begin() + ind, res.end());
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("strip")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.strip() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    size_t l = 0;
                    int r = (int)res.size() - 1;
                    while (l < res.size() && (int)r >= 0 && (res[l] == ' ' || res[r] == ' ')) {
                        l += res[l] == ' ';
                        r -= res[r] == ' ';
                    }
                    res = std::string(res.begin() + l, res.begin() + r + 1);
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("swapcase")) {
                    if(args.size() != 0) {
                        throw SemanticError("str.swapcase() takes no arguments",
                            x.base.base.loc);
                    }
                    AST::ConstantStr_t *n = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                    std::string res = n->m_value;
                    for (size_t i = 0; i < res.size(); i++)  {
                        char &cur = res[i];
                        if(cur >= 'a' && cur <= 'z') {
                            cur = cur -'a' + 'A';
                        } else if(cur >= 'A' && cur <= 'Z') {
                            cur = cur - 'A' + 'a';
                        }
                    }
                    ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 1, nullptr, nullptr , 0));
                    tmp = ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, res), str_type);
                    return;
                } else if (std::string(at->m_attr) == std::string("startswith")) {
                    if (args.size() != 1) {
                        throw SemanticError("str.startswith() takes one arguments",
                            x.base.base.loc);
                    }
                    ASR::expr_t *arg = args[0].m_value;
                    ASR::ttype_t *type = ASRUtils::expr_type(arg);
                    if (ASRUtils::is_character(*type)) {
                        AST::ConstantStr_t* str_str_con = AST::down_cast<AST::ConstantStr_t>(at->m_value);
                        std::string str = str_str_con->m_value;
                        if (ASRUtils::expr_value(arg) != nullptr) {
                            ASR::StringConstant_t* sub_str_con = ASR::down_cast<ASR::StringConstant_t>(arg);
                            std::string sub = sub_str_con->m_s;
                            size_t ind1 = 0, ind2 = 0;
                            bool res = !(str.size() == 0 && sub.size());
                            while ((ind1 < str.size()) && (ind2 < sub.size()) && res) {
                                res &= str[ind1] == sub[ind2];
                                ind1++;
                                ind2++;
                            }
                            res &= res? (ind2 == sub.size()): true;
                            tmp = ASR::make_LogicalConstant_t(al, x.base.base.loc, res, ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                                    4, nullptr, 0)));
                        } else {
                            ASR::symbol_t *fn_div = resolve_intrinsic_function(x.base.base.loc, "_lpython_str_startswith");
                            Vec<ASR::call_arg_t> args;
                            args.reserve(al, 1);
                            ASR::call_arg_t str_arg;
                            str_arg.loc = x.base.base.loc;
                            ASR::ttype_t *str_type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                                    1, 0, nullptr, nullptr, 0));
                            str_arg.m_value = ASRUtils::EXPR(
                                    ASR::make_StringConstant_t(al, x.base.base.loc, s2c(al, str), str_type));
                            ASR::call_arg_t sub_arg;
                            sub_arg.loc = x.base.base.loc;
                            sub_arg.m_value = arg;
                            args.push_back(al, str_arg);
                            args.push_back(al, sub_arg);
                            tmp = make_call_helper(al, fn_div, current_scope, args, "_lpython_str_startswith", x.base.base.loc);
                        }
                    } else {
                        throw SemanticError("str.startwith() takes one arguments of type: str",
                            arg->base.loc);
                    }
                    return;
                } else {
                    throw SemanticError("'str' object has no attribute '" + std::string(at->m_attr) + "'",
                        x.base.base.loc);
                }
            } else {
                throw SemanticError("Only Name type and constant integers supported in Call",
                    x.base.base.loc);
            }
        } else {
            throw SemanticError("Only Name or Attribute type supported in Call",
                x.base.base.loc);
        }

        ASR::symbol_t *s = current_scope->resolve_symbol(call_name);

        if (!s) {
            if (intrinsic_procedures.is_intrinsic(call_name)) {
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
            }
            if (false) {
                */
                // This will all be removed once we port it to intrinsic functions
            // Intrinsic functions
            if (call_name == "size") {
                // TODO: size should be part of ASR. That way
                // ASR itself does not need a runtime library
                // a runtime library thus becomes optional --- can either be
                // implemented using ASR, or the backend can link it at runtime
                ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                    4, nullptr, 0));
                /*
                ASR::symbol_t *a_name = nullptr;
                throw SemanticError("TODO: add the size() function and look it up",
                    x.base.base.loc);
                tmp = ASR::make_FunctionCall_t(al, x.base.base.loc, a_name,
                    nullptr, args.p, args.size(), nullptr, 0, a_type, nullptr, nullptr);
                */

                tmp = ASR::make_IntegerConstant_t(al, x.base.base.loc, 1234, a_type);
                return;
            } else if (call_name == "empty") {
                // TODO: check that the `empty` arguments are compatible
                // with the type
                tmp = nullptr;
                return;
            } else if (call_name == "empty_c_void_p") {
                // TODO: check that `empty_c_void_p uses` has arguments that are compatible
                // with the type
                tmp = nullptr;
                return;
            } else if (call_name == "TypeVar") {
                // Ignore TypeVar for now, we handle it based on the identifier itself
                tmp = nullptr;
                return;
            } else if (call_name == "callable") {
                if (args.size() != 1) {
                    throw SemanticError(call_name + "() takes exactly one argument (" +
                        std::to_string(args.size()) + " given)", x.base.base.loc);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                                    4, nullptr, 0));
                ASR::expr_t *arg = args[0].m_value;
                bool result = false;
                if (ASR::is_a<ASR::Var_t>(*arg)) {
                    ASR::symbol_t *t = ASR::down_cast<ASR::Var_t>(arg)->m_v;
                    result = ASR::is_a<ASR::Function_t>(*t);
                }
                tmp = ASR::make_LogicalConstant_t(al, x.base.base.loc, result, type);
                return;
            } else if( call_name == "pointer" ) {
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Pointer_t(al, x.base.base.loc,
                                            ASRUtils::expr_type(args[0].m_value)));
                tmp = ASR::make_GetPointer_t(al, x.base.base.loc, args[0].m_value, type, nullptr);
                return ;
            } else if( call_name == "array" ) {
                if( args.size() != 1 ) {
                    throw SemanticError("array accepts only 1 argument for now, got " +
                                        std::to_string(args.size()) + " arguments instead.",
                                        x.base.base.loc);
                }
                ASR::expr_t *arg = args[0].m_value;
                ASR::ttype_t *type = ASRUtils::expr_type(arg);
                if(ASR::is_a<ASR::ListConstant_t>(*arg)) {
                    type = ASR::down_cast<ASR::List_t>(type)->m_type;
                    ASR::ListConstant_t* list = ASR::down_cast<ASR::ListConstant_t>(arg);
                    ASR::expr_t **m_args = list->m_args;
                    size_t n_args = list->n_args;
                    tmp = ASR::make_ArrayConstant_t(al, x.base.base.loc, m_args, n_args, type);
                } else {
                    throw SemanticError("array accepts only list for now, got " +
                                        ASRUtils::type_to_str(type) + " type.", x.base.base.loc);
                }
                return;
            } else if( call_name == "deepcopy" ) {
                if( args.size() != 1 ) {
                    throw SemanticError("deepcopy only accepts one argument, found " +
                                        std::to_string(args.size()) + " instead.",
                                        x.base.base.loc);
                }
                tmp = (ASR::asr_t*) args[0].m_value;
                return ;
            } else if (intrinsic_node_handler.is_present(call_name)) {
                tmp = intrinsic_node_handler.get_intrinsic_node(call_name, al,
                                        x.base.base.loc, args, ann_assign_target_type);
                return;
            } else {
                // The function was not found and it is not intrinsic
                throw SemanticError("Function '" + call_name + "' is not declared and not intrinsic",
                    x.base.base.loc);
            }
            } // end of "comment"
        }
        tmp = make_call_helper(al, s, current_scope, args, call_name, x.base.base.loc,
                               false, x.m_args, x.n_args, x.m_keywords, x.n_keywords);
    }

    void visit_ImportFrom(const AST::ImportFrom_t &/*x*/) {
        // Handled by SymbolTableVisitor already
        tmp = nullptr;
    }
};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        const AST::Module_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit, bool main_module,
        std::map<int, ASR::symbol_t*> &ast_overload)
{
    BodyVisitor b(al, unit, diagnostics, main_module, ast_overload);
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

class PickleVisitor : public AST::PickleBaseVisitor<PickleVisitor>
{
public:
    std::string get_str() {
        return s;
    }
};

std::string pickle_python(AST::ast_t &ast, bool colors, bool indent) {
    PickleVisitor v;
    v.use_colors = colors;
    v.indent = indent;
    v.visit_ast(ast);
    return v.get_str();
}

class TreeVisitor : public AST::TreeBaseVisitor<TreeVisitor>
{
public:
    std::string get_str() {
        return s;
    }
};

std::string pickle_tree_python(AST::ast_t &ast, bool colors) {
    TreeVisitor v;
    v.use_colors = colors;
    v.visit_ast(ast);
    return v.get_str();
}

std::string get_parent_dir(const std::string &path) {
    int idx = path.size()-1;
    while (idx >= 0 && path[idx] != '/' && path[idx] != '\\') idx--;
    if (idx == -1) {
        return "";
    }
    return path.substr(0,idx);
}

Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
    AST::ast_t &ast, diag::Diagnostics &diagnostics, bool main_module,
    bool disable_main, bool symtab_only, std::string file_path)
{
    std::map<int, ASR::symbol_t*> ast_overload;
    std::string parent_dir = get_parent_dir(file_path);
    AST::Module_t *ast_m = AST::down_cast2<AST::Module_t>(&ast);

    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, *ast_m, diagnostics, main_module,
        ast_overload, parent_dir);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    if (!symtab_only) {
        auto res2 = body_visitor(al, *ast_m, diagnostics, unit, main_module,
            ast_overload);
        if (res2.ok) {
            tu = res2.result;
        } else {
            return res2.error;
        }
        LFORTRAN_ASSERT(asr_verify(*tu));
    }

    if (main_module) {
        // If it is a main module, turn it into a program
        // Note: we can modify this behavior for interactive mode later
        if (disable_main) {
            if (tu->n_items > 0) {
                diagnostics.add(diag::Diagnostic(
                    "The script is invoked as the main module and it has code to execute,\n"
                    "but `--disable-main` was passed so no code was generated for `main`.\n"
                    "We are removing all global executable code from ASR.",
                    diag::Level::Warning, diag::Stage::Semantic, {})
                );
                // We have to remove the code
                tu->m_items=nullptr;
                tu->n_items=0;
                LFORTRAN_ASSERT(asr_verify(*tu));
            }
        } else {
            LCompilers::PassOptions pass_options;
            pass_options.run_fun = "_lpython_main_program";
            pass_options.runtime_library_dir = get_runtime_library_dir();
            pass_wrap_global_stmts_into_program(al, *tu, pass_options);
            LFORTRAN_ASSERT(asr_verify(*tu));
        }
    }

    return tu;
}

} // namespace LFortran::Python
