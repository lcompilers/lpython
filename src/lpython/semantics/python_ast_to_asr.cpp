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

#include <lpython/python_ast.h>
#include <libasr/asr.h>
#include <libasr/asr_utils.h>
#include <libasr/asr_verify.h>
#include <lpython/semantics/python_ast_to_asr.h>
#include <libasr/string_utils.h>
#include <lpython/utils.h>
#include <lpython/semantics/semantic_exception.h>


namespace LFortran::Python {

ASR::Module_t* load_module(Allocator &/*al*/, SymbolTable *symtab,
                            const std::string &module_name,
                            const Location &loc, bool /*intrinsic*/,
                            const std::string &/*rl_path*/,
                            const std::function<void (const std::string &, const Location &)> err) {
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
    // TODO: load the module `module_name`.py, insert into `symtab` and return it
    return nullptr;
}

template <class Derived>
class CommonVisitor : public AST::BaseVisitor<Derived> {
public:
    diag::Diagnostics &diag;


    ASR::asr_t *tmp;
    Allocator &al;
    SymbolTable *current_scope;
    ASR::Module_t *current_module = nullptr;
    Vec<char *> current_module_dependencies;

    CommonVisitor(Allocator &al, SymbolTable *symbol_table,
            diag::Diagnostics &diagnostics)
        : diag{diagnostics}, al{al}, current_scope{symbol_table} {
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
        } else if (var_annotation == "c128") {
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
        diag::Diagnostics &diagnostics)
      : CommonVisitor(al, symbol_table, diagnostics), is_derived_type{false} {}


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
            ASR::abiType current_procedure_abi_type = ASR::abiType::Source;
            ASR::accessType s_access = ASR::accessType::Public;
            ASR::presenceType s_presence = ASR::presenceType::Required;
            bool value_attr = false;
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
            t = (ASR::symbol_t*)(load_module(al, current_scope,
                msym, x.base.base.loc, false, rl_path,
                [&](const std::string &msg, const Location &loc) { throw SemanticError(msg, loc); }
                ));
        }
//        ASR::Module_t *m = ASR::down_cast<ASR::Module_t>(t);

        tmp = nullptr;
    }

    void visit_AnnAssign(const AST::AnnAssign_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
    void visit_Assign(const AST::Assign_t &/*x*/) {
        // We skip this in the SymbolTable visitor, but visit it in the BodyVisitor
    }
};

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, const AST::Module_t &ast,
        diag::Diagnostics &diagnostics)
{
    SymbolTableVisitor v(al, nullptr, diagnostics);
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

    BodyVisitor(Allocator &al, ASR::asr_t *unit, diag::Diagnostics &diagnostics)
         : CommonVisitor(al, nullptr, diagnostics), asr{unit} {}

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

        Vec<ASR::asr_t*> items;
        items.reserve(al, 4);
        for (size_t i=0; i<x.n_body; i++) {
            tmp = nullptr;
            visit_stmt(*x.m_body[i]);
            if (tmp) {
                items.push_back(al, tmp);
            }
        }
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
        ASR::stmt_t *overloaded=nullptr;
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                overloaded);
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
        this->visit_expr(*x.m_slice);
        ASR::expr_t *index = ASRUtils::EXPR(tmp);
        Vec<ASR::array_index_t> args;
        args.reserve(al, 1);
        ASR::array_index_t ai;
        ai.loc = x.base.base.loc;
        ai.m_left = nullptr;
        ai.m_right = index;
        ai.m_step = nullptr;
        args.push_back(al, ai);

        ASR::symbol_t *s = ASR::down_cast<ASR::Var_t>(value)->m_v;
        ASR::Variable_t *v = ASR::down_cast<ASR::Variable_t>(s);
        ASR::ttype_t *type = v->m_type;
        tmp = ASR::make_ArrayRef_t(al, x.base.base.loc, s, args.p,
            args.size(), type, nullptr);
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
        } else if (ASRUtils::is_integer(*left_type) && ASRUtils::is_integer(*right_type)) {
            dest_type = left_type;
        } else if (ASRUtils::is_real(*left_type) && ASRUtils::is_real(*right_type)) {
            dest_type = left_type;
        } else if (ASRUtils::is_integer(*left_type) && ASRUtils::is_real(*right_type)) {
            // Cast LHS Integer->Real
            dest_type = right_type;
            left = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
        } else if (ASRUtils::is_real(*left_type) && ASRUtils::is_integer(*right_type)) {
            // Cast RHS Integer->Real
            dest_type = left_type;
            right = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
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
        ASR::expr_t *value = nullptr;

        if (ASRUtils::expr_value(operand) != nullptr) {
            if (ASRUtils::is_integer(*operand_type)) {

                int64_t op_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        ASRUtils::expr_value(operand))->m_n;
                if (op == ASR::unaryopType::Not) {
                    bool b = (op_value == 0) ? true : false;
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                        al, x.base.base.loc, b, operand_type));
                } else {
                    int64_t result;
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        case (ASR::unaryopType::Invert): { result = ~op_value; break; }
                        default: {
                            throw SemanticError("Bad operand type for unary operation",
                                x.base.base.loc);
                        }
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
                        al, x.base.base.loc, b, operand_type));
                } else {
                    double result;
                    switch (op) {
                        case (ASR::unaryopType::UAdd): { result = op_value; break; }
                        case (ASR::unaryopType::USub): { result = -op_value; break; }
                        default: {
                            throw SemanticError("Bad operand type for unary operation",
                                x.base.base.loc);
                        }
                    }
                    value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(
                        al, x.base.base.loc, result, operand_type));
                }

            } else if (ASRUtils::is_logical(*operand_type)) {
                bool op_value = ASR::down_cast<ASR::ConstantLogical_t>(
                                        ASRUtils::expr_value(operand))->m_value;
                bool result;
                if (op == ASR::unaryopType::Not) {
                    result = !op_value;
                } else {
                    throw SemanticError("Bad operand type for unary operation",
                        x.base.base.loc);
                }
                value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_ConstantLogical_t(al, x.base.base.loc, result, operand_type));

            } else if (ASRUtils::is_complex(*operand_type)) {
                ASR::ConstantComplex_t *c = ASR::down_cast<ASR::ConstantComplex_t>(
                                        ASRUtils::expr_value(operand));
                std::complex<double> op_value(c->m_re, c->m_im);
                std::complex<double> result;
                switch (op) {
                    case (ASR::unaryopType::UAdd): { result = op_value; break; }
                    case (ASR::unaryopType::USub): { result = -op_value; break; }
                    default: {
                        throw SemanticError("Bad operand type for unary operation",
                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(
                    ASR::make_ConstantComplex_t(al, x.base.base.loc,
                        std::real(result), std::imag(result), operand_type));
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
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, a_overloaded);

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
            right_type->type != ASR::ttypeType::Character))) {
        throw SemanticError(
            "Compare: only Integer or Real can be on the LHS and RHS."
            "If operator is Eq or NotEq then Complex type is also acceptable",
            x.base.base.loc);
        }

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
                    al, x.base.base.loc, result, source_type));
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
                    al, x.base.base.loc, result, source_type));
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
                        throw SemanticError("Comparison operator not implemented",
                                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantLogical_t(
                    al, x.base.base.loc, result, source_type));
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
        LFORTRAN_ASSERT(current_scope->scope.find(return_var_name) != current_scope->scope.end())
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
        if (AST::is_a<AST::Name_t>(*x.m_func)) {
            AST::Name_t *n = AST::down_cast<AST::Name_t>(x.m_func);
            call_name = n->m_id;
        } else {
            throw SemanticError("Only Name supported in Call",
                x.base.base.loc);
        }
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            visit_expr(*x.m_args[i]);
            ASR::expr_t *expr = ASRUtils::EXPR(tmp);
            args.push_back(al, expr);
        }

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

        } else if (call_name == "len") {
            // TODO(namannimmo10): make len work for lists, sets, tuples, etc. as well, once
            // they are supported.  For now, we just support len for strings.
            LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
            if (args.size() != 1) {
                throw SemanticError(call_name + "() takes exactly one argument (" +
                    std::to_string(args.size()) + " given)", x.base.base.loc);
            }
            ASR::expr_t *arg = ASRUtils::expr_value(args[0]);
            LFORTRAN_ASSERT(arg->type == ASR::exprType::ConstantString);
            ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Integer_t(al,
                                                          x.base.base.loc, 4, nullptr, 0));
            char* str_value = ASR::down_cast<ASR::ConstantString_t>(arg)->m_s;
            tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc,
                (int64_t)strlen(s2c(al, std::string(str_value))), type);
            return;

        } else if (call_name == "ord") {
            LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
            ASR::expr_t* char_expr = args[0];
            ASR::ttype_t* char_type = ASRUtils::expr_type(char_expr);
            if (ASRUtils::is_character(*char_type)) {
                char* c = ASR::down_cast<ASR::ConstantString_t>(ASRUtils::expr_value(char_expr))->m_s;
                ASR::ttype_t* int_type =
                    ASRUtils::TYPE(ASR::make_Integer_t(al,
                    x.base.base.loc, 4, nullptr, 0));
                tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc,
                    c[0], int_type);
                return;
            } else {
                throw SemanticError("ord() must have one character argument", x.base.base.loc);
            }
        } else if (call_name == "chr") {
            LFORTRAN_ASSERT(ASRUtils::all_args_evaluated(args));
            ASR::expr_t* real_expr = args[0];
            ASR::ttype_t* real_type = ASRUtils::expr_type(real_expr);
            if (ASRUtils::is_integer(*real_type)) {
                int64_t c = ASR::down_cast<ASR::ConstantInteger_t>(real_expr)->m_n;
                ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                    x.base.base.loc, 1, 1, nullptr, nullptr, 0));
                if (! (c >= 0 && c <= 127) ) {
                    throw SemanticError("The argument 'x' in chr(x) must be in the range 0 <= x <= 127.",
                                        x.base.base.loc);
                }
                char cc = c;
                std::string svalue;
                svalue += cc;
                tmp = ASR::make_ConstantString_t(al, x.base.base.loc, s2c(al, svalue), str_type);
                return;
            } else {
                throw SemanticError("chr() must have one integer argument", x.base.base.loc);
            }
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
        } else if (call_name == "pow") {
            if (args.size() != 2) {
                throw SemanticError("Two arguments are expected in pow",
                    x.base.base.loc);
            }
            ASR::expr_t *left = args[0];
            ASR::expr_t *right = args[1];
            ASR::binopType op = ASR::binopType::Pow;
            make_BinOp_helper(left, right, op, x.base.base.loc, false);
            return;
        } else if (call_name == "bin" || call_name == "oct" || call_name == "hex") {
            if (args.size() != 1) {
                throw SemanticError(call_name + "() takes exactly one argument (" +
                    std::to_string(args.size()) + " given)", x.base.base.loc);
            }
            ASR::expr_t* expr = args[0];
            ASR::ttype_t* type = ASRUtils::expr_type(expr);
            if (ASRUtils::is_integer(*type)) {
                int64_t n = ASR::down_cast<ASR::ConstantInteger_t>(expr)->m_n;
                ASR::ttype_t* str_type = ASRUtils::TYPE(ASR::make_Character_t(al,
                    x.base.base.loc, 1, 1, nullptr, nullptr, 0));
                std::string s, prefix;
                std::stringstream ss;
                if (call_name == "oct") {
                    prefix = "0o";
                    ss << std::oct << n;
                    s += ss.str();
                } else if (call_name == "bin") {
                    prefix = "0b";
                    s += std::bitset<64>(n).to_string();
                    s.erase(0, s.find_first_not_of('0'));
                } else {
                    prefix = "0x";
                    ss << std::hex << n;
                    s += ss.str();
                }
                s.insert(0, prefix);
                tmp = ASR::make_ConstantString_t(al, x.base.base.loc, s2c(al, s), str_type);
                return;
            } else {
                throw SemanticError(call_name + "() must have one integer argument",
                    x.base.base.loc);
            }
        } else if (call_name == "abs") {
            if (args.size() != 1) {
                throw SemanticError(call_name + "() takes exactly one argument (" +
                    std::to_string(args.size()) + " given)", x.base.base.loc);
            }
            ASR::expr_t* arg = args[0];
            ASR::ttype_t* t = ASRUtils::expr_type(arg);
            if (ASRUtils::is_real(*t)) {
                double rv = ASR::down_cast<ASR::ConstantReal_t>(arg)->m_r;
                double val = std::abs(rv);
                tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, val, t);
            } else if (ASRUtils::is_integer(*t)) {
                int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
                int64_t val = std::abs(rv);
                tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, val, t);
            } else if (ASRUtils::is_complex(*t)) {
                double re = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_re;
                double im = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_im;
                std::complex<double> c(re, im);
                double result = std::abs(c);
                tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, result, t);
            } else if (ASRUtils::is_logical(*t)) {
                bool rv = ASR::down_cast<ASR::ConstantLogical_t>(arg)->m_value;
                int8_t val = rv ? 1 : 0;
                tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, val, t);
            } else {
                throw SemanticError(call_name + "() must have one real, integer, complex, or logical argument",
                    x.base.base.loc);
            }
            return;
        } else if (call_name == "bool") {
            if (args.size() != 1) {
                throw SemanticError(call_name + "() takes exactly one argument (" +
                    std::to_string(args.size()) + " given)", x.base.base.loc);
            }
            ASR::ttype_t *type = ASRUtils::TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                                  1, nullptr, 0));
            ASR::expr_t* arg = args[0];
            ASR::ttype_t* t = ASRUtils::expr_type(arg);
            bool result;
            if (ASRUtils::is_real(*t)) {
                double rv = ASR::down_cast<ASR::ConstantReal_t>(arg)->m_r;
                result = rv ? true : false;
            } else if (ASRUtils::is_integer(*t)) {
                int64_t rv = ASR::down_cast<ASR::ConstantInteger_t>(arg)->m_n;
                result = rv ? true : false;
            } else if (ASRUtils::is_complex(*t)) {
                double re = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_re;
                double im = ASR::down_cast<ASR::ConstantComplex_t>(arg)->m_im;
                std::complex<double> c(re, im);
                result = (re || im) ? true : false;
            } else if (ASRUtils::is_logical(*t)) {
                bool rv = ASR::down_cast<ASR::ConstantLogical_t>(arg)->m_value;
                result = rv;
            } else if (ASRUtils::is_character(*t)) {
                char* c = ASR::down_cast<ASR::ConstantString_t>(ASRUtils::expr_value(arg))->m_s;
                result = strlen(s2c(al, std::string(c))) ? true : false;
            } else {
                throw SemanticError(call_name + "() must have one real, integer, character,"
                        " complex, or logical argument", x.base.base.loc);
            }
            tmp = ASR::make_ConstantLogical_t(al, x.base.base.loc, result, type);
            return;
        }

        // Other functions
        ASR::symbol_t *s = current_scope->resolve_symbol(call_name);
        if (!s) {
            throw SemanticError("Function '" + call_name + "' is not declared",
                x.base.base.loc);
        }
        ASR::ttype_t *a_type = ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            4, nullptr, 0));
        tmp = ASR::make_FunctionCall_t(al, x.base.base.loc, s,
            nullptr, args.p, args.size(), nullptr, 0, a_type, nullptr, nullptr);
    }

    void visit_ImportFrom(const AST::ImportFrom_t &/*x*/) {
        // Handled by SymbolTableVisitor already
        tmp = nullptr;
    }
};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        const AST::Module_t &ast,
        diag::Diagnostics &diagnostics,
        ASR::asr_t *unit)
{
    BodyVisitor b(al, unit, diagnostics);
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
    AST::ast_t &ast, diag::Diagnostics &diagnostics)
{
    AST::Module_t *ast_m = AST::down_cast2<AST::Module_t>(&ast);

    ASR::asr_t *unit;
    auto res = symbol_table_visitor(al, *ast_m, diagnostics);
    if (res.ok) {
        unit = res.result;
    } else {
        return res.error;
    }
    ASR::TranslationUnit_t *tu = ASR::down_cast2<ASR::TranslationUnit_t>(unit);
    LFORTRAN_ASSERT(asr_verify(*tu));

    auto res2 = body_visitor(al, *ast_m, diagnostics, unit);
    if (res2.ok) {
        tu = res2.result;
    } else {
        return res2.error;
    }
    LFORTRAN_ASSERT(asr_verify(*tu));

    return tu;
}

} // namespace LFortran::Python
