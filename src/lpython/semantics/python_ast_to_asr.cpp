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
#include <libasr/string_utils.h>
#include <libasr/utils.h>
#include <libasr/pass/global_stmts_program.h>

#include <lpython/python_ast.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>
#include <lpython/python_serialization.h>
#include <lpython/semantics/python_comptime_eval.h>


namespace LFortran::Python {

LFortran::Result<LFortran::Python::AST::ast_t*> parse_python_file(Allocator &al,
        const std::string &runtime_library_dir,
        const std::string &infile) {
    std::string pycmd = "python " + runtime_library_dir + "/lpython_parser.py " + infile;
    int err = std::system(pycmd.c_str());
    if (err != 0) {
        std::cerr << "The command '" << pycmd << "' failed." << std::endl;
        return LFortran::Error();
    }
    std::string infile_ser = "ser.txt";
    std::string input;
    bool status = read_file(infile_ser, input);
    if (!status) {
        std::cerr << "The file '" << infile_ser << "' cannot be read." << std::endl;
        return LFortran::Error();
    }
    LFortran::Python::AST::ast_t* ast = LFortran::Python::deserialize_ast(al, input);
    return ast;
}

// Does a CPython style lookup for a module:
// * First the current directory (this is incorrect, we need to do it relative to the current file)
// * Then the LPython runtime directory
LFortran::Result<std::string> get_full_path(const std::string &filename,
        const std::string &runtime_library_dir, bool &ltypes) {
    ltypes = false;
    std::string input;
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
            } else {
                return LFortran::Error();
            }
        }
    }
}

ASR::Module_t* load_module(Allocator &al, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool intrinsic,
                            const std::string &rl_path,
                            bool &ltypes,
                            const std::function<void (const std::string &, const Location &)> err) {
    ltypes = false;
    LFORTRAN_ASSERT(symtab);
    if (symtab->scope.find(module_name) != symtab->scope.end()) {
        ASR::symbol_t *m = symtab->scope[module_name];
        if (ASR::is_a<ASR::Module_t>(*m)) {
            return ASR::down_cast<ASR::Module_t>(m);
        } else {
            err("The symbol '" + module_name + "' is not a module", loc);
        }
    }
    LFORTRAN_ASSERT(symtab->parent == nullptr);

    // Parse the module `module_name`.py to AST
    std::string infile0 = module_name + ".py";
    Result<std::string> rinfile = get_full_path(infile0, rl_path, ltypes);
    if (!rinfile.ok) {
        err("Could not find the module '" + infile0 + "'", loc);
    }
    if (ltypes) return nullptr;
    std::string infile = rinfile.result;
    Result<AST::ast_t*> r = parse_python_file(al, rl_path, infile);
    if (!r.ok) {
        err("The file '" + infile + "' failed to parse", loc);
    }
    LFortran::Python::AST::ast_t* ast = r.result;

    // Convert the module from AST to ASR
    LFortran::LocationManager lm;
    lm.in_filename = infile;
    // TODO: diagnostic should be an argument to this function
    diag::Diagnostics diagnostics;
    Result<ASR::TranslationUnit_t*> r2 = python_ast_to_asr(al, *ast, diagnostics, false);
    std::string input;
    read_file(infile, input);
    CompilerOptions compiler_options;
    std::cerr << diagnostics.render(input, lm, compiler_options);
    if (!r2.ok) {
        LFORTRAN_ASSERT(diagnostics.has_error())
        return nullptr; // Error
    }
    ASR::TranslationUnit_t* mod1 = r2.result;

    // insert into `symtab`
    ASR::Module_t *mod2 = ASRUtils::extract_module(*mod1);
    mod2->m_name = s2c(al, module_name);
    symtab->scope[module_name] = (ASR::symbol_t*)mod2;
    mod2->m_symtab->parent = symtab;
    mod2->m_intrinsic = intrinsic;

    // and return it
    return mod2;
}

template <class Derived>
class CommonVisitor : public AST::BaseVisitor<Derived> {
public:
    diag::Diagnostics &diag;


    ASR::asr_t *tmp;
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

    CommonVisitor(Allocator &al, SymbolTable *symbol_table,
            diag::Diagnostics &diagnostics, bool main_module)
        : diag{diagnostics}, al{al}, current_scope{symbol_table}, main_module{main_module} {
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
        if( v->type == ASR::symbolType::Variable ) {
            ASR::Variable_t* v_var = ASR::down_cast<ASR::Variable_t>(v);
            if( v_var->m_type == nullptr &&
                v_var->m_intent == ASR::intentType::AssociateBlock ) {
                return (ASR::asr_t*)(v_var->m_symbolic_value);
            }
        }
        return ASR::make_Var_t(al, loc, v);
    }

    ASR::symbol_t* resolve_intrinsic_function(const Location &loc, const std::string &remote_sym) {
        LFORTRAN_ASSERT(intrinsic_procedures.is_intrinsic(remote_sym))
        std::string module_name = intrinsic_procedures.get_module(remote_sym, loc);

        SymbolTable *tu_symtab = ASRUtils::get_tu_symtab(current_scope);
        std::string rl_path = get_runtime_library_dir();
        bool ltypes;
        ASR::Module_t *m = load_module(al, tu_symtab, module_name,
                loc, true, rl_path,
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
                    || ASR::is_a<ASR::Subroutine_t>(*t))) {
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

        current_scope->scope[sym] = ASR::down_cast<ASR::symbol_t>(fn);
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

    // Convert Python AST type annotation to an ASR type
    // Examples:
    // i32, i64, f32, f64
    // f64[256], i32[:]
    ASR::ttype_t * ast_expr_to_asr_type(const Location &loc, const AST::expr_t &annotation) {
        Vec<ASR::dimension_t> dims;
        dims.reserve(al, 4);

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
                if (AST::is_a<AST::Name_t>(*s->m_slice)) {
                    type = ast_expr_to_asr_type(loc, *s->m_slice);
                } else if (AST::is_a<AST::Tuple_t>(*s->m_slice)) {
                    AST::Tuple_t *t = AST::down_cast<AST::Tuple_t>(s->m_slice);
                    type = ast_expr_to_asr_type(loc, *t->m_elts[0]);
                } else {
                    throw SemanticError("Only Name or Tuple in Subscript supported for now in `list`"
                        " annotation", loc);
                }
                return type;
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
            } else {
                ASR::dimension_t dim;
                dim.loc = loc;
                if (AST::is_a<AST::Slice_t>(*s->m_slice)) {
                    dim.m_start = nullptr;
                    dim.m_end = nullptr;
                } else {
                    this->visit_expr(*s->m_slice);
                    ASR::expr_t *value = ASRUtils::EXPR(tmp);
                    if (!ASR::is_a<ASR::ConstantInteger_t>(*value)) {
                        throw SemanticError("Only Integer in [] in Subscript supported for now in annotation",
                            loc);
                    }
                    ASR::ttype_t *itype = ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                            4, nullptr, 0));
                    dim.m_start = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, loc, 1, itype));
                    dim.m_end = value;
                }

                dims.push_back(al, dim);
            }
        } else {
            throw SemanticError("Only Name or Subscript supported for now in annotation of annotated assignment.",
                loc);
        }

        ASR::ttype_t *type;
        if (var_annotation == "i32") {
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
                1, dims.p, dims.size()));
        } else {
            throw SemanticError("Unsupported type annotation: " + var_annotation, loc);
        }
        return type;
    }

};

ASR::symbol_t* import_from_module(Allocator &al, ASR::Module_t *m, SymbolTable *current_scope,
                std::string mname, std::string cur_sym_name, std::string new_sym_name,
                const Location &loc) {

    ASR::symbol_t *t = m->m_symtab->resolve_symbol(cur_sym_name);
    if (!t) {
        throw SemanticError("The symbol '" + cur_sym_name + "' not found in the module '" + mname + "'",
                loc);
    }
    if (current_scope->scope.find(cur_sym_name) != current_scope->scope.end()) {
        throw SemanticError(cur_sym_name + " already defined", loc);
    }
    if (ASR::is_a<ASR::Subroutine_t>(*t)) {

        ASR::Subroutine_t *msub = ASR::down_cast<ASR::Subroutine_t>(t);
        // `msub` is the Subroutine in a module. Now we construct
        // an ExternalSymbol that points to
        // `msub` via the `external` field.
        Str name;
        name.from_str(al, new_sym_name);
        ASR::asr_t *sub = ASR::make_ExternalSymbol_t(
            al, msub->base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ name.c_str(al),
            (ASR::symbol_t*)msub,
            m->m_name, nullptr, 0, msub->m_name,
            ASR::accessType::Public
            );
        return ASR::down_cast<ASR::symbol_t>(sub);
    } else if (ASR::is_a<ASR::Function_t>(*t)) {
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
    } else {
        throw SemanticError("Only Subroutines, Functions and Variables are currently supported in 'import'",
            loc);
    }
    // should not reach here
    return nullptr;
}

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
    Vec<char*> data_member_names;
    std::vector<std::string> current_procedure_args;
    ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
    std::map<SymbolTable*, ASR::accessType> assgn;
    ASR::symbol_t *current_module_sym;
    std::vector<std::string> excluded_from_symtab;


    SymbolTableVisitor(Allocator &al, SymbolTable *symbol_table,
        diag::Diagnostics &diagnostics, bool main_module)
      : CommonVisitor(al, symbol_table, diagnostics, main_module), is_derived_type{false} {}


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

            if (parent_scope->scope.find(mod_name) != parent_scope->scope.end()) {
                throw SemanticError("Module '" + mod_name + "' already defined", tmp1->loc);
            }
            parent_scope->scope[mod_name] = ASR::down_cast<ASR::symbol_t>(tmp1);
        }

        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
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
        if (x.n_decorator_list == 1) {
            AST::expr_t *dec = x.m_decorator_list[0];
            if (AST::is_a<AST::Name_t>(*dec)) {
                std::string name = AST::down_cast<AST::Name_t>(dec)->m_id;
                if (name == "ccall") {
                    current_procedure_abi_type = ASR::abiType::BindC;
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
            current_scope->scope[arg_s] = ASR::down_cast<ASR::symbol_t>(v);

            ASR::symbol_t *var = current_scope->scope[arg_s];
            args.push_back(al, ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Subroutine already defined", tmp->loc);
        }
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        if (current_procedure_abi_type == ASR::abiType::BindC) {
            deftype = ASR::deftypeType::Interface;
        }
        char *bindc_name=nullptr;
        if (x.m_returns) {
            if (AST::is_a<AST::Name_t>(*x.m_returns)) {
                std::string return_var_name = "_lpython_return_variable";
                ASR::ttype_t *type = ast_expr_to_asr_type(x.m_returns->base.loc, *x.m_returns);
                ASR::asr_t *return_var = ASR::make_Variable_t(al, x.m_returns->base.loc,
                    current_scope, s2c(al, return_var_name), ASRUtils::intent_return_var, nullptr, nullptr,
                    ASR::storage_typeType::Default, type,
                    current_procedure_abi_type, ASR::Public, ASR::presenceType::Required,
                    false);
                LFORTRAN_ASSERT(current_scope->scope.find(return_var_name) == current_scope->scope.end())
                current_scope->scope[return_var_name]
                         = ASR::down_cast<ASR::symbol_t>(return_var);
                ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
                    current_scope->scope[return_var_name]);
                tmp = ASR::make_Function_t(
                    al, x.base.base.loc,
                    /* a_symtab */ current_scope,
                    /* a_name */ s2c(al, sym_name),
                    /* a_args */ args.p,
                    /* n_args */ args.size(),
                    /* a_body */ nullptr,
                    /* n_body */ 0,
                    /* a_return_var */ ASRUtils::EXPR(return_var_ref),
                    current_procedure_abi_type,
                    s_access, deftype, bindc_name);

            } else {
                throw SemanticError("Return variable must be an identifier",
                    x.m_returns->base.loc);
            }
        } else {
            bool is_pure = false, is_module = false;
            tmp = ASR::make_Subroutine_t(
                al, x.base.base.loc,
                /* a_symtab */ current_scope,
                /* a_name */ s2c(al, sym_name),
                /* a_args */ args.p,
                /* n_args */ args.size(),
                /* a_body */ nullptr,
                /* n_body */ 0,
                current_procedure_abi_type,
                s_access, deftype, bindc_name,
                is_pure, is_module);
        }
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(tmp);
        current_scope = parent_scope;
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
            if (!main_module) {
                st = st->parent;
            }
            bool ltypes;
            t = (ASR::symbol_t*)(load_module(al, st,
                msym, x.base.base.loc, false, rl_path, ltypes,
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
        }

        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);
        for (auto &remote_sym : mod_symbols) {
            ASR::symbol_t *t = import_from_module(al, m, current_scope, msym,
                                remote_sym, remote_sym, x.base.base.loc);
            current_scope->scope[remote_sym] = t;
        }

        tmp = nullptr;
    }

    void visit_Import(const AST::Import_t &x) {
        ASR::symbol_t *t = nullptr;
        std::string rl_path = get_runtime_library_dir();
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
                mod_sym, x.base.base.loc, false, rl_path, ltypes,
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
    void visit_Assign(const AST::Assign_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
    void visit_Expr(const AST::Expr_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
};

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, const AST::Module_t &ast,
        diag::Diagnostics &diagnostics, bool main_module)
{
    SymbolTableVisitor v(al, nullptr, diagnostics, main_module);
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
    Vec<ASR::stmt_t*> *current_body;

    BodyVisitor(Allocator &al, ASR::asr_t *unit, diag::Diagnostics &diagnostics, bool main_module)
         : CommonVisitor(al, nullptr, diagnostics, main_module), asr{unit} {}

    // Transforms statements to a list of ASR statements
    // In addition, it also inserts the following nodes if needed:
    //   * ImplicitDeallocate
    //   * GoToTarget
    // The `body` Vec must already be reserved
    void transform_stmts(Vec<ASR::stmt_t*> &body, size_t n_body, AST::stmt_t **m_body) {
        tmp = nullptr;
        for (size_t i=0; i<n_body; i++) {
            // Visit the statement
            current_body = &body;
            this->visit_stmt(*m_body[i]);
            current_body = nullptr;
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = ASRUtils::STMT(tmp);
                body.push_back(al, tmp_stmt);
            }
            // To avoid last statement to be entered twice once we exit this node
            tmp = nullptr;
        }
    }

    void visit_Module(const AST::Module_t &x) {
        ASR::TranslationUnit_t *unit = ASR::down_cast2<ASR::TranslationUnit_t>(asr);
        current_scope = unit->m_global_scope;
        LFORTRAN_ASSERT(current_scope != nullptr);
        if (!main_module) {
            ASR::Module_t *mod = ASR::down_cast<ASR::Module_t>(current_scope->scope["__main__"]);
            current_scope = mod->m_symtab;
            LFORTRAN_ASSERT(current_scope != nullptr);
        }

        Vec<ASR::asr_t*> items;
        items.reserve(al, 4);
        for (size_t i=0; i<x.n_body; i++) {
            tmp = nullptr;
            visit_stmt(*x.m_body[i]);
            if (tmp) {
                items.push_back(al, tmp);
            }
        }
        // These global statements are added to the translation unit for now,
        // but they should be adding to a module initialization function
        unit->m_items = items.p;
        unit->n_items = items.size();

        tmp = asr;
    }

    template <typename Procedure>
    void handle_fn(const AST::FunctionDef_t &x, Procedure &v) {
        current_scope = v.m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        v.m_body = body.p;
        v.n_body = body.size();
    }

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[x.m_name];
        if (ASR::is_a<ASR::Subroutine_t>(*t)) {
            handle_fn(x, *ASR::down_cast<ASR::Subroutine_t>(t));
        } else if (ASR::is_a<ASR::Function_t>(*t)) {
            ASR::Function_t *f = ASR::down_cast<ASR::Function_t>(t);
            handle_fn(x, *f);
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

        if (current_scope->scope.find(var_name) !=
                current_scope->scope.end()) {
            if (current_scope->parent != nullptr) {
                // Re-declaring a global scope variable is allowed,
                // otherwise raise an error
                ASR::symbol_t *orig_decl = current_scope->scope[var_name];
                throw SemanticError(diag::Diagnostic(
                    "Symbol is already declared in the same scope",
                    diag::Level::Error, diag::Stage::Semantic, {
                        diag::Label("original declaration", {orig_decl->base.loc}, false),
                        diag::Label("redeclaration", {x.base.base.loc}),
                    }));
            }
        }

        ASR::ttype_t *type = ast_expr_to_asr_type(x.base.base.loc, *x.m_annotation);

        ASR::expr_t *value = nullptr;
        if (x.m_value) {
            this->visit_expr(*x.m_value);
            value = ASRUtils::EXPR(tmp);
            value = implicitcast_helper(type, value, true);
        }
        ASR::expr_t *init_expr = nullptr;
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
        current_scope->scope[var_name] = ASR::down_cast<ASR::symbol_t>(v);

        tmp = nullptr;
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

    // Casts `right` if needed to the type of `left`
    // (to be used during assignment, BinOp, or compare)
    ASR::expr_t* implicitcast_helper(ASR::ttype_t *left_type, ASR::expr_t *right,
                                        bool is_assign=false) {
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        if (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) {
            bool is_l64 = ASR::down_cast<ASR::Integer_t>(left_type)->m_kind == 8;
            bool is_r64 = ASR::down_cast<ASR::Integer_t>(right_type)->m_kind == 8;
            if (is_l64 != is_r64) {
                return ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                    al, right->base.loc, right, ASR::cast_kindType::IntegerToInteger,
                    left_type, nullptr));
            }
        } else if (ASRUtils::is_real(*left_type) && ASRUtils::is_real(*right_type)) {
            bool is_l64 = ASR::down_cast<ASR::Real_t>(left_type)->m_kind == 8;
            bool is_r64 = ASR::down_cast<ASR::Real_t>(right_type)->m_kind == 8;
            if (is_l64 != is_r64) {
                return ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                    al, right->base.loc, right, ASR::cast_kindType::RealToReal,
                    left_type, nullptr));
            }
        } else if (!is_assign && ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
            return ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, right->base.loc, right, ASR::cast_kindType::IntegerToReal,
                left_type, nullptr));
        } else if (is_assign && ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
            throw SemanticError("Assigning integer to float is not supported",
                    right->base.loc);
        }
        return right;
    }

    void visit_Assign(const AST::Assign_t &x) {
        ASR::expr_t *target;
        if (x.n_targets == 1) {
            this->visit_expr(*x.m_targets[0]);
            target = ASRUtils::EXPR(tmp);
        } else {
            throw SemanticError("Assignment to multiple targets not supported",
                x.base.base.loc);
        }
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        value = implicitcast_helper(target_type, value, true);
        ASR::stmt_t *overloaded=nullptr;
        bool realloc_lhs = false;
        if (ASR::is_a<ASR::Character_t>(*target_type)) {
            ASR::Character_t *t = ASR::down_cast<ASR::Character_t>(target_type);
            if (t->m_len == -2) {
                // Reallocate LHS for allocatable strings
                realloc_lhs = true;
            }
        }
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                overloaded, realloc_lhs);
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

    void visit_Subscript(const AST::Subscript_t &x) {
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = ASRUtils::EXPR(tmp);
        Vec<ASR::array_index_t> args;
        args.reserve(al, 1);
        ASR::array_index_t ai;
        ai.loc = x.base.base.loc;
        ai.m_left = nullptr;
        ai.m_right = nullptr;
        ai.m_step = nullptr;
        if (AST::is_a<AST::Slice_t>(*x.m_slice)) {
            AST::Slice_t *s = AST::down_cast<AST::Slice_t>(x.m_slice);
            if (s->m_lower != nullptr) {
                this->visit_expr(*s->m_lower);
                ai.m_left = ASRUtils::EXPR(tmp);
            }
            if (s->m_upper != nullptr) {
                this->visit_expr(*s->m_upper);
                ai.m_right = ASRUtils::EXPR(tmp);
            }
            if (s->m_step != nullptr) {
                this->visit_expr(*s->m_step);
                ai.m_step = ASRUtils::EXPR(tmp);
            }
        } else {
            this->visit_expr(*x.m_slice);
            ASR::expr_t *index = ASRUtils::EXPR(tmp);
            ai.m_right = index;
        }
        args.push_back(al, ai);
        ASR::symbol_t *s = ASR::down_cast<ASR::Var_t>(value)->m_v;
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        tmp = ASR::make_ArrayRef_t(al, x.base.base.loc, s, args.p,
            args.size(), type, nullptr);
    }

    void visit_List(const AST::List_t &x) {
        Vec<ASR::expr_t*> list;
        list.reserve(al, x.n_elts);
        ASR::ttype_t *type = nullptr;
        for (size_t i=0; i<x.n_elts; i++) {
            this->visit_expr(*x.m_elts[i]);
            ASR::expr_t *expr = ASRUtils::EXPR(tmp);
            if (type == nullptr) {
                type = ASRUtils::expr_type(expr);
            } else {
                if (!ASRUtils::check_equal_type(ASRUtils::expr_type(expr), type)) {
                    throw SemanticError("All List elements must be of the same type for now",
                        x.base.base.loc);
                }
            }
            list.push_back(al, expr);
        }
        tmp = ASR::make_ConstantArray_t(al, x.base.base.loc, list.p,
            list.size(), type);
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
        tmp = ASR::make_ConstantTuple_t(al, x.base.base.loc,
                                    elements.p, elements.size(), tuple_type);
    }

    void visit_For(const AST::For_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target=ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        ASR::expr_t *loop_end = nullptr, *loop_start = nullptr, *inc = nullptr;
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

        } else {
            throw SemanticError("Only function call `range(..)` supported as for loop iteration for now",
                x.base.base.loc);
        }

        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            4, nullptr, 0));
        ASR::expr_t *constant_one = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
                                            al, x.base.base.loc, 1, a_type));
        make_BinOp_helper(loop_end, constant_one, ASR::binopType::Sub,
                            x.base.base.loc, false);
        loop_end = ASRUtils::EXPR(tmp);
        ASR::do_loop_head_t head;
        head.m_v = target;
        if (loop_start) {
            head.m_start = loop_start;
        } else {
            head.m_start = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, 0, a_type));
        }
        head.m_end = loop_end;
        if (inc) {
            head.m_increment = inc;
        } else {
            head.m_increment = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(al, x.base.base.loc, 1, a_type));
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

    void visit_Name(const AST::Name_t &x) {
        std::string name = x.m_id;
        ASR::symbol_t *s = current_scope->resolve_symbol(name);
        if (s) {
            tmp = ASR::make_Var_t(al, x.base.base.loc, s);
        } else {
            throw SemanticError("Variable '" + name + "' not declared",
                x.base.base.loc);
        }
    }

    void visit_ConstantInt(const AST::ConstantInt_t &x) {
        int64_t i = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, i, type);
    }

    void visit_ConstantFloat(const AST::ConstantFloat_t &x) {
        double f = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, f, type);
    }

    void visit_ConstantComplex(const AST::ConstantComplex_t &x) {
        double re = x.m_re, im = x.m_im;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_ConstantComplex_t(al, x.base.base.loc, re, im, type);
    }

    void visit_ConstantStr(const AST::ConstantStr_t &x) {
        char *s = x.m_value;
        size_t s_size = std::string(s).size();
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                1, s_size, nullptr, nullptr, 0));
        tmp = ASR::make_ConstantString_t(al, x.base.base.loc, s, type);
    }

    void visit_ConstantBool(const AST::ConstantBool_t &x) {
        bool b = x.m_value;
        ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                1, nullptr, 0));
        tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, b, type);
    }

    void visit_BoolOp(const AST::BoolOp_t &x) {
        ASR::boolopType op;
        if (x.n_values > 2) {
            throw SemanticError("Only two operands supported for boolean operations",
                x.base.base.loc);
        }
        this->visit_expr(*x.m_values[0]);
        ASR::expr_t *lhs = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_values[1]);
        ASR::expr_t *rhs = ASRUtils::EXPR(tmp);
        switch (x.m_op) {
            case (AST::boolopType::And): { op = ASR::boolopType::And; break; }
            case (AST::boolopType::Or): { op = ASR::boolopType::Or; break; }
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
            bool left_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                    ASRUtils::expr_value(lhs))->m_value;
            bool right_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                    ASRUtils::expr_value(rhs))->m_value;
            bool result;
            switch (op) {
                case (ASR::boolopType::And): { result = left_value && right_value; break; }
                case (ASR::boolopType::Or): { result = left_value || right_value; break; }
                default : {
                    throw SemanticError("Boolean operator type not supported",
                        x.base.base.loc);
                }
            }
            value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                al, x.base.base.loc, result, dest_type));
        }
        tmp = ASR::make_BoolOp_t(al, x.base.base.loc, lhs, op, rhs, dest_type, value);
    }

    void make_BinOp_helper(ASR::expr_t *left, ASR::expr_t *right,
                            ASR::binopType op, const Location &loc, bool floordiv) {
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;

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
                    dest_type = ASRUtils::TYPE(ASR::make_Integer_t(al,
                        loc, 4, nullptr, 0));
                } else {
                    dest_type = ASRUtils::TYPE(ASR::make_Real_t(al,
                        loc, 8, nullptr, 0));
                }
                if (ASRUtils::is_real(*left_type)) {
                    left = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                        al, left->base.loc, left, ASR::cast_kindType::RealToInteger, dest_type,
                        value));
                }
                if (ASRUtils::is_real(*right_type)) {
                    right = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                        al, right->base.loc, right, ASR::cast_kindType::RealToInteger, dest_type,
                        value));
                }

            } else { // real divison in python using (`/`)
                dest_type = ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                    8, nullptr, 0));
                if (ASRUtils::is_integer(*left_type)) {
                    left = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                        al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type,
                        value));
                }
                if (ASRUtils::is_integer(*right_type)) {
                    right = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                        al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type,
                        value));
                }
            }
        } else if((ASRUtils::is_integer(*left_type) || ASRUtils::is_real(*left_type)) &&
                    (ASRUtils::is_integer(*right_type) || ASRUtils::is_real(*right_type)) ){
            left = implicitcast_helper(ASRUtils::expr_type(right), left);
            right = implicitcast_helper(ASRUtils::expr_type(left), right);
            dest_type = ASRUtils::expr_type(left);
        } else if ((right_is_int || left_is_int) && op == ASR::binopType::Mul) {
            // string repeat
            ASR::stropType ops = ASR::stropType::Repeat;
            int64_t left_int = 0, right_int = 0, dest_len = 0;
            if (right_is_int) {
                ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
                LFORTRAN_ASSERT(left_type2->n_dims == 0);
                right_int = ASR::down_cast<ASR::ConstantInteger_t>(
                                                   ASRUtils::expr_value(right))->m_n;
                dest_len = left_type2->m_len * right_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, left_type2->m_kind,
                        dest_len, nullptr, nullptr, 0));
            } else if (left_is_int) {
                ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
                LFORTRAN_ASSERT(right_type2->n_dims == 0);
                left_int = ASR::down_cast<ASR::ConstantInteger_t>(
                                                   ASRUtils::expr_value(left))->m_n;
                dest_len = right_type2->m_len * left_int;
                if (dest_len < 0) dest_len = 0;
                dest_type = ASR::down_cast<ASR::ttype_t>(
                        ASR::make_Character_t(al, loc, right_type2->m_kind,
                        dest_len, nullptr, nullptr, 0));
            }

            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* str = right_is_int ? ASR::down_cast<ASR::ConstantString_t>(
                                                ASRUtils::expr_value(left))->m_s :
                                                ASR::down_cast<ASR::ConstantString_t>(
                                                ASRUtils::expr_value(right))->m_s;
                int64_t repeat = right_is_int ? right_int : left_int;
                char* result;
                std::ostringstream os;
                std::fill_n(std::ostream_iterator<std::string>(os), repeat, std::string(str));
                result = s2c(al, os.str());
                LFORTRAN_ASSERT((int64_t)strlen(result) == dest_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(
                    al, loc, result, dest_type));
            }
            tmp = ASR::make_StrOp_t(al, loc, left, ops, right, dest_type, value);
            return;

        } else if (ASRUtils::is_character(*left_type) && ASRUtils::is_character(*right_type)
                            && op == ASR::binopType::Add) {
            // string concat
            ASR::stropType ops = ASR::stropType::Concat;
            ASR::Character_t *left_type2 = ASR::down_cast<ASR::Character_t>(left_type);
            ASR::Character_t *right_type2 = ASR::down_cast<ASR::Character_t>(right_type);
            LFORTRAN_ASSERT(left_type2->n_dims == 0);
            LFORTRAN_ASSERT(right_type2->n_dims == 0);
            dest_type = ASR::down_cast<ASR::ttype_t>(
                    ASR::make_Character_t(al, loc, left_type2->m_kind,
                    left_type2->m_len + right_type2->m_len, nullptr, nullptr, 0));
            if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
                char* left_value = ASR::down_cast<ASR::ConstantString_t>(
                                        ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::ConstantString_t>(
                                        ASRUtils::expr_value(right))->m_s;
                char* result;
                std::string result_s = std::string(left_value) + std::string(right_value);
                result = s2c(al, result_s);
                LFORTRAN_ASSERT((int64_t)strlen(result) == ASR::down_cast<ASR::Character_t>(dest_type)->m_len)
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantString_t(
                    al, loc, result, dest_type));
            }
            tmp = ASR::make_StrOp_t(al, loc, left, ops, right, dest_type,
                                    value);
            return;

        } else if (ASRUtils::is_complex(*left_type) && ASRUtils::is_complex(*right_type)) {
            dest_type = left_type;
        } else {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Not Implemented: type mismatch in binary operator; only Integer/Real combinations "
                "and string concatenation/repetition are implemented for now",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }

        // Check that the types are now the same
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(left),
                                    ASRUtils::expr_type(right))) {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Type mismatch in binary operator, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }
        // Now, compute the result of the binary operations
        if (ASRUtils::expr_value(left) != nullptr && ASRUtils::expr_value(right) != nullptr) {
            if (ASRUtils::is_integer(*dest_type)) {
                int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                                    ASRUtils::expr_value(left))->m_n;
                int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                                    ASRUtils::expr_value(right))->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::binopType::Add): { result = left_value + right_value; break; }
                    case (ASR::binopType::Sub): { result = left_value - right_value; break; }
                    case (ASR::binopType::Mul): { result = left_value * right_value; break; }
                    case (ASR::binopType::Div): { result = left_value / right_value; break; }
                    case (ASR::binopType::Pow): { result = std::pow(left_value, right_value); break; }
                    default: { LFORTRAN_ASSERT(false); } // should never happen
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
                    al, loc, result, dest_type));
            }
            else if (ASRUtils::is_real(*dest_type)) {
                double left_value = ASR::down_cast<ASR::ConstantReal_t>(
                                                    ASRUtils::expr_value(left))->m_r;
                double right_value = ASR::down_cast<ASR::ConstantReal_t>(
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(
                    al, loc, result, dest_type));
            }
            else if (ASRUtils::is_complex(*dest_type)) {
                ASR::ConstantComplex_t *left0 = ASR::down_cast<ASR::ConstantComplex_t>(
                                                                ASRUtils::expr_value(left));
                ASR::ConstantComplex_t *right0 = ASR::down_cast<ASR::ConstantComplex_t>(
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantComplex_t(al, loc,
                        std::real(result), std::imag(result), dest_type));
            }
        }
        ASR::expr_t *overloaded = nullptr;
        tmp = ASR::make_BinOp_t(al, loc, left, op, right, dest_type,
                                value, overloaded);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);
        ASR::binopType op;
        switch (x.m_op) {
            case (AST::operatorType::Add) : { op = ASR::binopType::Add; break; }
            case (AST::operatorType::Sub) : { op = ASR::binopType::Sub; break; }
            case (AST::operatorType::Mult) : { op = ASR::binopType::Mul; break; }
            case (AST::operatorType::Div) : { op = ASR::binopType::Div; break; }
            case (AST::operatorType::FloorDiv) : {op = ASR::binopType::Div; break;}
            case (AST::operatorType::Pow) : { op = ASR::binopType::Pow; break; }
            default : {
                throw SemanticError("Binary operator type not supported",
                    x.base.base.loc);
            }
        }
        bool floordiv = (x.m_op == AST::operatorType::FloorDiv);
        make_BinOp_helper(left, right, op, x.base.base.loc, floordiv);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = ASRUtils::EXPR(tmp);
        ASR::unaryopType op;
        switch (x.m_op) {
            case (AST::unaryopType::Invert) : { op = ASR::unaryopType::Invert; break; }
            case (AST::unaryopType::Not) : { op = ASR::unaryopType::Not; break; }
            case (AST::unaryopType::UAdd) : { op = ASR::unaryopType::UAdd; break; }
            case (AST::unaryopType::USub) : { op = ASR::unaryopType::USub; break; }
            default : {
                throw SemanticError("Unary operator type not supported",
                    x.base.base.loc);
            }
        }
        ASR::ttype_t *operand_type = ASRUtils::expr_type(operand);
        ASR::ttype_t *logical_type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::ttype_t *int_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        ASR::expr_t *value = nullptr;

        if (ASRUtils::expr_value(operand) != nullptr) {
            if (ASRUtils::is_integer(*operand_type)) {

                int64_t op_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        ASRUtils::expr_value(operand))->m_n;
                if (op == ASR::unaryopType::Not) {
                    bool b = (op_value == 0) ? true : false;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                        al, x.base.base.loc, b, logical_type));
                } else {
                    int64_t result = 0;
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        case (ASR::unaryopType::Invert): { result = ~op_value; break; }
                        default: LFORTRAN_ASSERT(false); // should never happen
                    }
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
                                al, x.base.base.loc, result, operand_type));
                }

            } else if (ASRUtils::is_real(*operand_type)) {
                double op_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        ASRUtils::expr_value(operand))->m_r;
                if (op == ASR::unaryopType::Not) {
                    bool b = (op_value == 0.0) ? true : false;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                        al, x.base.base.loc, b, logical_type));
                } else {
                    double result = 0.0;
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        default: {
                            throw SemanticError("Bad operand type for unary " +
                                ASRUtils::unop_to_str(op) + ": " + ASRUtils::type_to_str(operand_type),
                                x.base.base.loc);
                        }
                    }
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(
                        al, x.base.base.loc, result, operand_type));
                }

            } else if (ASRUtils::is_logical(*operand_type)) {
                bool op_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                               ASRUtils::expr_value(operand))->m_value;
                if (op == ASR::unaryopType::Not) {
                    value = ASR::down_cast<ASR::expr_t>(
                        ASR::make_ConstantLogical_t(al, x.base.base.loc, !op_value, logical_type));
                } else {
                    int8_t result = 0;
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = +op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        case (ASR::unaryopType::Invert): { result = op_value ? -2 : -1; break; }
                        default : LFORTRAN_ASSERT(false); // should never happen
                    }
                    value = ASR::down_cast<ASR::expr_t>(
                        ASR::make_ConstantInteger_t(al, x.base.base.loc, result, int_type));
                }

            } else if (ASRUtils::is_complex(*operand_type)) {
                ASR::ConstantComplex_t *c = ASR::down_cast<ASR::ConstantComplex_t>(
                                        ASRUtils::expr_value(operand));
                std::complex<double> op_value(c->m_re, c->m_im);
                std::complex<double> result;
                if (op == ASR::unaryopType::Not) {
                    bool b = (op_value.real() == 0.0 && op_value.imag() == 0.0) ? true : false;
                    value = ASR::down_cast<ASR::expr_t>(
                        ASR::make_ConstantLogical_t(al, x.base.base.loc, b, logical_type));
                } else {
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        default: {
                            throw SemanticError("Bad operand type for unary " +
                                ASRUtils::unop_to_str(op) + ": " + ASRUtils::type_to_str(operand_type),
                                x.base.base.loc);
                        }
                    }
                    value = ASR::down_cast<ASR::expr_t>(
                        ASR::make_ConstantComplex_t(al, x.base.base.loc,
                        std::real(result), std::imag(result), operand_type));
                }
            }
        }
        tmp = ASR::make_UnaryOp_t(al, x.base.base.loc, op, operand, operand_type,
                              value);
    }

    void visit_AugAssign(const AST::AugAssign_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *left = ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *right = ASRUtils::EXPR(tmp);
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
        ASR::stmt_t* a_overloaded = nullptr;
        ASR::expr_t *tmp2 = ASR::down_cast<ASR::expr_t>(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, a_overloaded, false);

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

    void visit_IfExp(const AST::IfExp_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = ASRUtils::EXPR(tmp);
        visit_expr(*x.m_body);
        ASR::expr_t *body = ASRUtils::EXPR(tmp);
        visit_expr(*x.m_orelse);
        ASR::expr_t *orelse = ASRUtils::EXPR(tmp);
        LFORTRAN_ASSERT(ASRUtils::check_equal_type(ASRUtils::expr_type(body),
                                                   ASRUtils::expr_type(orelse)));
        tmp = ASR::make_IfExp_t(al, x.base.base.loc, test, body, orelse,
                                ASRUtils::expr_type(body));
    }

    void visit_Dict(const AST::Dict_t &x) {
        LFORTRAN_ASSERT(x.n_keys == x.n_values);
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
        tmp = ASR::make_ConstantDictionary_t(al, x.base.base.loc, keys.p, keys.size(),
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
        left = implicitcast_helper(ASRUtils::expr_type(right), left);
        right = implicitcast_helper(ASRUtils::expr_type(left), right);
        // Check that the types are now the same
        if (!ASRUtils::check_equal_type(ASRUtils::expr_type(left),
                                    ASRUtils::expr_type(right))) {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Type mismatch in comparison operator, the types must be compatible",
                diag::Level::Error, diag::Stage::Semantic, {
                    diag::Label("type mismatch (" + ltype + " and " + rtype + ")",
                            {left->base.loc, right->base.loc})
                })
            );
            throw SemanticAbort();
        }
        ASR::ttype_t *type = ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::expr_t *value = nullptr;
        ASR::ttype_t *source_type = left_type;

        // Now, compute the result
        if (ASRUtils::expr_value(left) != nullptr &&
            ASRUtils::expr_value(right) != nullptr) {
            if (ASRUtils::is_integer(*source_type)) {
                int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        ASRUtils::expr_value(left))
                                        ->m_n;
                int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        ASRUtils::expr_value(right))
                                        ->m_n;
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, type));
            } else if (ASRUtils::is_real(*source_type)) {
                double left_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        ASRUtils::expr_value(left))
                                        ->m_r;
                double right_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        ASRUtils::expr_value(right))
                                        ->m_r;
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, type));
            } else if (ASR::is_a<ASR::Complex_t>(*source_type)) {
                ASR::ConstantComplex_t *left0
                    = ASR::down_cast<ASR::ConstantComplex_t>(ASRUtils::expr_value(left));
                ASR::ConstantComplex_t *right0
                    = ASR::down_cast<ASR::ConstantComplex_t>(ASRUtils::expr_value(right));
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, type));

            } else if (ASRUtils::is_logical(*source_type)) {
                bool left_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                        ASRUtils::expr_value(left))->m_value;
                bool right_value = ASR::down_cast<ASR::ConstantLogical_t>(
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, type));

            } else if (ASRUtils::is_character(*source_type)) {
                char* left_value = ASR::down_cast<ASR::ConstantString_t>(
                                        ASRUtils::expr_value(left))->m_s;
                char* right_value = ASR::down_cast<ASR::ConstantString_t>(
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
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, type));
            }
        }
        tmp = ASR::make_Compare_t(al, x.base.base.loc, left, asr_op, right,
                                  type, value, overloaded);
    }

    void visit_Pass(const AST::Pass_t &/*x*/) {
        tmp = nullptr;
    }

    void visit_Return(const AST::Return_t &x) {
        std::string return_var_name = "_lpython_return_variable";
        if(current_scope->scope.find(return_var_name) == current_scope->scope.end()) {
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
        ASR::symbol_t *return_var = current_scope->scope[return_var_name];
        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc, return_var);
        ASR::expr_t *target = ASRUtils::EXPR(return_var_ref);
        ASR::ttype_t *target_type = ASRUtils::expr_type(target);
        ASR::ttype_t *value_type = ASRUtils::expr_type(value);
        if (!ASRUtils::check_equal_type(target_type, value_type)) {
            std::string ltype = ASRUtils::type_to_str(target_type);
            std::string rtype = ASRUtils::type_to_str(value_type);
            throw SemanticError("Type Mismatch in return, found (" +
                    ltype + " and " + rtype + ")", x.base.base.loc);
        }
        value = implicitcast_helper(ASRUtils::expr_type(target), value, true);
        ASR::stmt_t *overloaded=nullptr;
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                overloaded, false);

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
        tmp = ASR::make_ConstantSet_t(al, x.base.base.loc, elements.p, elements.size(), set_type);
    }

    void visit_Expr(const AST::Expr_t &x) {
        if (AST::is_a<AST::Call_t>(*x.m_value)) {
            AST::Call_t *c = AST::down_cast<AST::Call_t>(x.m_value);
            std::string call_name;
            if (AST::is_a<AST::Name_t>(*c->m_func)) {
                AST::Name_t *n = AST::down_cast<AST::Name_t>(c->m_func);
                call_name = n->m_id;
            } else {
                throw SemanticError("Only Name supported in Call",
                    x.base.base.loc);
            }
            Vec<ASR::expr_t*> args;
            args.reserve(al, c->n_args);
            for (size_t i=0; i<c->n_args; i++) {
                visit_expr(*c->m_args[i]);
                ASR::expr_t *expr = ASRUtils::EXPR(tmp);
                args.push_back(al, expr);
            }
            if (call_name == "print") {
                ASR::expr_t *fmt=nullptr;
                tmp = ASR::make_Print_t(al, x.base.base.loc, fmt,
                    args.p, args.size());
                return;

            } else if (call_name == "quit") {
                ASR::expr_t *code;
                if (args.size() == 0) {
                    code = nullptr;
                } else if (args.size() == 1) {
                    code = args[0];
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
            tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc, s,
                nullptr, args.p, args.size(), nullptr);
            return;
        }
        this->visit_expr(*x.m_value);
        ASRUtils::EXPR(tmp);
        tmp = nullptr;
    }

    void visit_Call(const AST::Call_t &x) {
        std::string call_name;
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            ASR::expr_t *expr = ASRUtils::EXPR(tmp);
            args.push_back(al, expr);
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
                if (current_scope->scope.find(call_name_store) != current_scope->scope.end()) {
                    st = current_scope->scope[call_name_store];
                } else {
                    SymbolTable *symtab = current_scope;
                    while (symtab->parent != nullptr) symtab = symtab->parent;
                    if (symtab->scope.find(mod_name) == symtab->scope.end()) {
                        throw SemanticError("module '" + mod_name + "' is not imported",
                            x.base.base.loc);
                    }
                    ASR::symbol_t *mt = symtab->scope[mod_name];
                    ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(mt);
                    st = import_from_module(al, m, current_scope, mod_name,
                                        call_name, call_name_store, x.base.base.loc);
                    current_scope->scope[call_name_store] = st;
                }
                ASR::symbol_t *stemp = st;
                st = ASRUtils::symbol_get_past_external(st);

                if(ASR::is_a<ASR::Function_t>(*st)) {
                    ASR::Function_t *func = ASR::down_cast<ASR::Function_t>(st);
                    ASR::ttype_t *a_type = ASRUtils::expr_type(func->m_return_var);
                    tmp = ASR::make_FunctionCall_t(al, x.base.base.loc, stemp,
                        nullptr, args.p, args.size(), nullptr, 0, a_type, nullptr, nullptr);
                } else if(ASR::is_a<ASR::Subroutine_t>(*st)) {
                    tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc, stemp,
                        nullptr, args.p, args.size(), nullptr);
                } else {
                    throw SemanticError("Unsupported call type for " + call_name,
                        x.base.base.loc);
                }
                return;
            } else {
                throw SemanticError("Only Name type supported in Call",
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

                tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, 1234, a_type);
                return;

            } else if (call_name == "complex") {
                int16_t n_args = args.size();
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Complex_t(al, x.base.base.loc,
                    8, nullptr, 0));
                if( n_args > 2 || n_args < 0 ) { // n_args shouldn't be less than 0 but added this check for safety
                    throw SemanticError("Only constant integer or real values are supported as "
                        "the (at most two) arguments of complex()", x.base.base.loc);
                }
                double c1 = 0.0, c2 = 0.0; // Default if n_args = 0
                if (n_args >= 1) { // Handles both n_args = 1 and n_args = 2
                    if (ASR::is_a<ASR::ConstantInteger_t>(*args[0])) {
                        c1 = ASR::down_cast<ASR::ConstantInteger_t>(args[0])->m_n;
                    } else if (ASR::is_a<ASR::ConstantReal_t>(*args[0])) {
                        c1 = ASR::down_cast<ASR::ConstantReal_t>(ASRUtils::expr_value(args[0]))->m_r;
                    }
                }
                if (n_args == 2) { // Extracts imaginary component if n_args = 2
                    if (ASR::is_a<ASR::ConstantInteger_t>(*args[1])) {
                        c2 = ASR::down_cast<ASR::ConstantInteger_t>(args[1])->m_n;
                    } else if (ASR::is_a<ASR::ConstantReal_t>(*args[1])) {
                        c2 = ASR::down_cast<ASR::ConstantReal_t>(ASRUtils::expr_value(args[1]))->m_r;
                    }
                }
                tmp = ASR::make_ConstantComplex_t(al, x.base.base.loc, c1, c2, type);
                return;
            } else if (call_name == "callable") {
                if (args.size() != 1) {
                    throw SemanticError(call_name + "() takes exactly one argument (" +
                        std::to_string(args.size()) + " given)", x.base.base.loc);
                }
                ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                                    1, nullptr, 0));
                ASR::expr_t *arg = args[0];
                bool result = false;
                if (ASR::is_a<ASR::Var_t>(*arg)) {
                    ASR::symbol_t *t = ASR::down_cast<ASR::Var_t>(arg)->m_v;
                    result = (ASR::is_a<ASR::Function_t>(*t) ||
                                ASR::is_a<ASR::Subroutine_t>(*t));
                }
                tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, result, type);
                return;
            } else if (call_name == "divmod") {
                if (args.size() != 2) {
                    throw SemanticError(call_name + "() takes exactly two arguments (" +
                        std::to_string(args.size()) + " given)", x.base.base.loc);
                }
                ASR::expr_t* arg1 = ASRUtils::expr_value(args[0]);
                ASR::expr_t* arg2 = ASRUtils::expr_value(args[1]);
                ASR::ttype_t* arg1_type = ASRUtils::expr_type(arg1);
                ASR::ttype_t* arg2_type = ASRUtils::expr_type(arg2);
                Vec<ASR::expr_t*> tuple; // pair consisting of quotient and remainder
                tuple.reserve(al, 2);
                Vec<ASR::ttype_t*> tuple_type_vec;
                tuple_type_vec.reserve(al, 2);
                if (arg1 == nullptr || arg2 == nullptr) {
                    throw SemanticError("runtime divmod(x, y) is not supported, only compile time for now",
                        x.base.base.loc);
                }
                if (ASRUtils::is_integer(*arg1_type) && ASRUtils::is_integer(*arg2_type)) {
                    int64_t ival1 = ASR::down_cast<ASR::ConstantInteger_t>(arg1)->m_n;
                    int64_t ival2 = ASR::down_cast<ASR::ConstantInteger_t>(arg2)->m_n;
                    if (ival2 == 0) {
                        throw SemanticError("Integer division or modulo by zero not possible",
                            x.base.base.loc);
                    } else {
                        int64_t div = ival1 / ival2;
                        int64_t mod = ival1 % ival2;
                        tuple.push_back(al, ASRUtils::EXPR(
                            ASR::make_ConstantInteger_t(al, x.base.base.loc, div, arg1_type)));
                        tuple.push_back(al, ASRUtils::EXPR(
                            ASR::make_ConstantInteger_t(al, x.base.base.loc, mod, arg1_type)));
                        tuple_type_vec.push_back(al, arg1_type);
                        tuple_type_vec.push_back(al, arg2_type);
                        ASR::ttype_t *tuple_type = ASRUtils::TYPE(ASR::make_Tuple_t(al, x.base.base.loc,
                                                    tuple_type_vec.p, tuple_type_vec.n));
                        tmp = ASR::make_ConstantTuple_t(al, x.base.base.loc, tuple.p, tuple.size(), tuple_type);
                        return;
                    }
                } else {
                    throw SemanticError("Both arguments of divmod() must be integers, not '" +
                        ASRUtils::type_to_str(arg1_type) + "' and '" + ASRUtils::type_to_str(arg2_type) + "'",
                        x.base.base.loc);
                }
            } else {
                // The function was not found and it is not intrinsic
                throw SemanticError("Function '" + call_name + "' is not declared and not intrinsic",
                    x.base.base.loc);
            }
            } // end of "comment"
        }

        // handling ExternalSymbol
        ASR::symbol_t *stemp = s;
        s = ASRUtils::symbol_get_past_external(s);

        if(ASR::is_a<ASR::Function_t>(*s)) {
            ASR::Function_t *func = ASR::down_cast<ASR::Function_t>(s);
            ASR::ttype_t *a_type = ASRUtils::expr_type(func->m_return_var);
            ASR::expr_t *value = nullptr;
            if (ASRUtils::is_intrinsic_function2(func)) {
                value = intrinsic_procedures.comptime_eval(call_name, al, x.base.base.loc, args);
            }
            tmp = ASR::make_FunctionCall_t(al, x.base.base.loc, stemp,
                nullptr, args.p, args.size(), nullptr, 0, a_type, value, nullptr);
        } else if(ASR::is_a<ASR::Subroutine_t>(*s)) {
            tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc, stemp,
                nullptr, args.p, args.size(), nullptr);
        } else {
            throw SemanticError("Unsupported call type for " + call_name,
                x.base.base.loc);
        }
    }

    void visit_ImportFrom(const AST::ImportFrom_t &/*x*/) {
        // Handled by SymbolTableVisitor already
        tmp = nullptr;
    }
};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        const AST::Module_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit, bool main_module)
{
    BodyVisitor b(al, unit, diagnostics, main_module);
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

Result<ASR::TranslationUnit_t*> python_ast_to_asr(Allocator &al,
    AST::ast_t &ast, diag::Diagnostics &diagnostics, bool main_module)
{
    AST::Module_t *ast_m = AST::down_cast2<AST::Module_t>(&ast);

    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, *ast_m, diagnostics, main_module);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    auto res2 = body_visitor(al, *ast_m, diagnostics, unit, main_module);
    if (res2.ok) {
        tu = res2.result;
    } else {
        return res2.error;
    }
    LFORTRAN_ASSERT(asr_verify(*tu));

    if (main_module) {
        // If it is a main module, turn it into a program.
        // Note: we can modify this behavior for interactive mode later
        pass_wrap_global_stmts_into_program(al, *tu);
        LFORTRAN_ASSERT(asr_verify(*tu));
    }

    return tu;
}

} // namespace LFortran::Python
