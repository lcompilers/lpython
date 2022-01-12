#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <cmath>
#include <vector>

#include <lfortran/python_ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>
#include <lfortran/semantics/python_ast_to_asr.h>
#include <lfortran/string_utils.h>
#include <lfortran/utils.h>


namespace LFortran::Python {

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
                ASR::ttype_t *itype = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
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
            type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "i64") {
            type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, loc,
                8, dims.p, dims.size()));
        } else if (var_annotation == "f32") {
            type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                4, dims.p, dims.size()));
        } else if (var_annotation == "f64") {
            type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, loc,
                8, dims.p, dims.size()));
        } else if (var_annotation == "str") {
            type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, loc,
                1, -2, nullptr, dims.p, dims.size()));
        } else if (var_annotation == "bool") {
            type = LFortran::ASRUtils::TYPE(ASR::make_Logical_t(al, loc,
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
            ASR::intentType s_intent = LFortran::ASRUtils::intent_in;
            if (ASRUtils::is_array(arg_type)) {
                s_intent = LFortran::ASRUtils::intent_inout;
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
            args.push_back(al, LFortran::ASRUtils::EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Subroutine already defined", tmp->loc);
        }
        bool is_pure = false, is_module = false;
        ASR::accessType s_access = ASR::accessType::Public;
        ASR::deftypeType deftype = ASR::deftypeType::Implementation;
        char *bindc_name=nullptr;
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
        parent_scope->scope[sym_name] = ASR::down_cast<ASR::symbol_t>(tmp);
        current_scope = parent_scope;
    }
};

Result<ASR::asr_t*> symbol_table_visitor(Allocator &al, AST::Module_t &ast,
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
            this->visit_stmt(*m_body[i]);
            if (tmp != nullptr) {
                ASR::stmt_t* tmp_stmt = LFortran::ASRUtils::STMT(tmp);
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

        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
        }

        tmp = asr;
    }

    void visit_FunctionDef(const AST::FunctionDef_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::symbol_t *t = current_scope->scope[x.m_name];
        ASR::Subroutine_t *v = ASR::down_cast<ASR::Subroutine_t>(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        v->m_body = body.p;
        v->n_body = body.size();
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
                        diag::Label("redeclaration", {x.base.base.loc}),
                        diag::Label("original declaration", {orig_decl->base.loc}, false),
                    }));
            }
        }

        ASR::ttype_t *type = ast_expr_to_asr_type(x.base.base.loc, *x.m_annotation);

        ASR::expr_t *value = nullptr;
        ASR::expr_t *init_expr = nullptr;
        ASR::intentType s_intent = LFortran::ASRUtils::intent_local;
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
                ASR::symbol_t *s = current_scope->scope[var_name];
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
            target = LFortran::ASRUtils::EXPR(tmp);
        } else {
            throw SemanticError("Assignment to multiple targets not supported",
                x.base.base.loc);
        }
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = LFortran::ASRUtils::EXPR(tmp);
        ASR::stmt_t *overloaded=nullptr;
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value,
                                overloaded);
    }

    void visit_Subscript(const AST::Subscript_t &x) {
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_slice);
        ASR::expr_t *index = LFortran::ASRUtils::EXPR(tmp);
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
                ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
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

        ASR::ttype_t *a_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            4, nullptr, 0));
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
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantInteger_t(al, x.base.base.loc, i, type);
    }

    void visit_ConstantFloat(const AST::ConstantFloat_t &x) {
        double f = x.m_value;
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Real_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, f, type);
    }

    void visit_ConstantStr(const AST::ConstantStr_t &x) {
        char *s = x.m_value;
        size_t s_size = std::string(s).size();
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(ASR::make_Character_t(al, x.base.base.loc,
                1, s_size, nullptr, nullptr, 0));
        tmp = ASR::make_ConstantString_t(al, x.base.base.loc, s, type);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
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
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;

        // Cast LHS or RHS if necessary
        if (ASR::is_a<ASR::Integer_t>(*left_type) && ASR::is_a<ASR::Integer_t>(*right_type)) {
            dest_type = left_type;
        } else if (ASR::is_a<ASR::Real_t>(*left_type) && ASR::is_a<ASR::Real_t>(*right_type)) {
            dest_type = left_type;
        } else if (ASR::is_a<ASR::Integer_t>(*left_type) && ASR::is_a<ASR::Real_t>(*right_type)) {
            // Cast LHS Integer->Real
            dest_type = right_type;
            left = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
        } else if (ASR::is_a<ASR::Real_t>(*left_type) && ASR::is_a<ASR::Integer_t>(*right_type)) {
            // Cast RHS Integer->Real
            dest_type = left_type;
            right = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
        } else if (ASR::is_a<ASR::Character_t>(*left_type) && ASR::is_a<ASR::Character_t>(*right_type)
                            && op == ASR::binopType::Add) {
            // string concat
            dest_type = left_type;
            ASR::stropType ops = ASR::stropType::Concat;
            tmp = ASR::make_StrOp_t(al, x.base.base.loc, left, ops, right, dest_type,
                                    value);
            return;
        } else {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Not Implemented: type mismatch in binary operator, only Integer/Real combinations "
                "and string concatenation is implemented for now",
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
        ASR::expr_t *overloaded = nullptr;
        tmp = ASR::make_BinOp_t(al, x.base.base.loc, left, op, right, dest_type,
                                value, overloaded);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = LFortran::ASRUtils::EXPR(tmp);
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
        ASR::ttype_t *operand_type = LFortran::ASRUtils::expr_type(operand);
        ASR::expr_t *value = nullptr;

        if (LFortran::ASRUtils::expr_value(operand) != nullptr) {
            if (ASR::is_a<LFortran::ASR::Integer_t>(*operand_type)) {
                int64_t op_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        LFortran::ASRUtils::expr_value(operand))
                                        ->m_n;
                int64_t result;
                switch (op) {
                    case (ASR::unaryopType::UAdd): { result = op_value; break; }
                    case (ASR::unaryopType::USub): { result = -op_value; break; }
                    default: {
                        throw SemanticError("Unary operator not implemented yet for compile time evaluation",
                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantInteger_t(
                            al, x.base.base.loc, result, operand_type));
            } else if (ASR::is_a<LFortran::ASR::Real_t>(*operand_type)) {
                double op_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        LFortran::ASRUtils::expr_value(operand))
                                        ->m_r;
                double result;
                switch (op) {
                    case (ASR::unaryopType::UAdd): { result = op_value; break; }
                    case (ASR::unaryopType::USub): { result = -op_value; break; }
                    default: {
                        throw SemanticError("Unary operator not implemented yet for compile time evaluation",
                            x.base.base.loc);
                    }
                }
                value = ASR::down_cast<ASR::expr_t>(ASR::make_ConstantReal_t(
                            al, x.base.base.loc, result, operand_type));
            }
        }
        tmp = ASR::make_UnaryOp_t(al, x.base.base.loc, op, operand, operand_type,
                              value);
    }

    void visit_AugAssign(const AST::AugAssign_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);
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
        ASR::ttype_t *left_type = ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = ASRUtils::expr_type(right);
        ASR::ttype_t *dest_type = nullptr;
        ASR::expr_t *value = nullptr;

        // Cast LHS or RHS if necessary
        if (ASR::is_a<ASR::Integer_t>(*left_type) && ASR::is_a<ASR::Integer_t>(*right_type)) {
            dest_type = left_type;
        } else if (ASR::is_a<ASR::Real_t>(*left_type) && ASR::is_a<ASR::Real_t>(*right_type)) {
            dest_type = left_type;
        } else if (ASR::is_a<ASR::Integer_t>(*left_type) && ASR::is_a<ASR::Real_t>(*right_type)) {
            // Cast LHS Integer->Real
            dest_type = right_type;
            left = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, left->base.loc, left, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
        } else if (ASR::is_a<ASR::Real_t>(*left_type) && ASR::is_a<ASR::Integer_t>(*right_type)) {
            // Cast RHS Integer->Real
            dest_type = left_type;
            right = ASR::down_cast<ASR::expr_t>(ASR::make_ImplicitCast_t(
                al, right->base.loc, right, ASR::cast_kindType::IntegerToReal, dest_type,
                value));
        } else if (ASR::is_a<ASR::Character_t>(*left_type) && ASR::is_a<ASR::Character_t>(*right_type)
                            && op == ASR::binopType::Add) {
            // string concat
            dest_type = left_type;
            ASR::stropType ops = ASR::stropType::Concat;
            tmp = ASR::make_StrOp_t(al, x.base.base.loc, left, ops, right, dest_type,
                                    value);
            ASR::expr_t *tmp2 = ASR::down_cast<ASR::expr_t>(tmp);
            tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, nullptr);
            return;
        } else {
            std::string ltype = ASRUtils::type_to_str(ASRUtils::expr_type(left));
            std::string rtype = ASRUtils::type_to_str(ASRUtils::expr_type(right));
            diag.add(diag::Diagnostic(
                "Not Implemented: type mismatch in binary operator, only Integer/Real combinations implemented for now",
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
        ASR::expr_t *overloaded = nullptr;
        ASR::stmt_t* a_overloaded = nullptr;
        tmp = ASR::make_BinOp_t(al, x.base.base.loc, left, op, right, dest_type,
                         value, overloaded);
        ASR::expr_t *tmp2 = ASR::down_cast<ASR::expr_t>(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, left, tmp2, a_overloaded);

    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        transform_stmts(orelse, x.n_orelse, x.m_orelse);
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_While(const AST::While_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = LFortran::ASRUtils::EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        transform_stmts(body, x.n_body, x.m_body);
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, test, body.p,
                body.size());
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = LFortran::ASRUtils::EXPR(tmp);
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
        ASR::expr_t *right = LFortran::ASRUtils::EXPR(tmp);

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

        ASR::ttype_t *left_type = LFortran::ASRUtils::expr_type(left);
        ASR::ttype_t *right_type = LFortran::ASRUtils::expr_type(right);
        ASR::expr_t *overloaded = nullptr;
        if (((left_type->type != ASR::ttypeType::Real &&
            left_type->type != ASR::ttypeType::Integer) &&
            (right_type->type != ASR::ttypeType::Real &&
            right_type->type != ASR::ttypeType::Integer) &&
            (left_type->type != ASR::ttypeType::Character ||
            right_type->type != ASR::ttypeType::Character))
            && overloaded == nullptr) {
        throw SemanticError(
            "Compare: only Integer or Real can be on the LHS and RHS.",
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
        ASR::ttype_t *type = LFortran::ASRUtils::TYPE(
            ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::expr_t *value = nullptr;
        ASR::ttype_t *source_type = left_type;

        // Now, compute the result
        if (LFortran::ASRUtils::expr_value(left) != nullptr &&
            LFortran::ASRUtils::expr_value(right) != nullptr) {
            if (ASR::is_a<LFortran::ASR::Integer_t>(*source_type)) {
                int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        LFortran::ASRUtils::expr_value(left))
                                        ->m_n;
                int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                        LFortran::ASRUtils::expr_value(right))
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
            } else if (ASR::is_a<LFortran::ASR::Real_t>(*source_type)) {
                double left_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        LFortran::ASRUtils::expr_value(left))
                                        ->m_r;
                double right_value = ASR::down_cast<ASR::ConstantReal_t>(
                                        LFortran::ASRUtils::expr_value(right))
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
            }
        }
        tmp = ASR::make_Compare_t(al, x.base.base.loc, left, asr_op, right,
                                  type, value, overloaded);
    }

    void visit_Return(const AST::Return_t &x) {
        tmp = ASR::make_Return_t(al, x.base.base.loc);
    }

    void visit_Continue(const AST::Continue_t &x) {
        tmp = ASR::make_Cycle_t(al, x.base.base.loc);
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
                ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
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
        LFortran::ASRUtils::EXPR(tmp);
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
            ASR::expr_t *expr = LFortran::ASRUtils::EXPR(tmp);
            args.push_back(al, expr);
        }

        // Intrinsic functions
        if (call_name == "size") {
            // TODO: size should be part of ASR. That way
            // ASR itself does not need a runtime library
            // a runtime library thus becomes optional --- can either be
            // implemented using ASR, or the backend can link it at runtime
            ASR::ttype_t *a_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
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
        }

        // Other functions
        ASR::symbol_t *s = current_scope->resolve_symbol(call_name);
        if (!s) {
            throw SemanticError("Function '" + call_name + "' is not declared",
                x.base.base.loc);
        }
        ASR::ttype_t *a_type = LFortran::ASRUtils::TYPE(ASR::make_Integer_t(al, x.base.base.loc,
            4, nullptr, 0));
        tmp = ASR::make_FunctionCall_t(al, x.base.base.loc, s,
            nullptr, args.p, args.size(), nullptr, 0, a_type, nullptr, nullptr);
    }
};

Result<ASR::TranslationUnit_t*> body_visitor(Allocator &al,
        AST::Module_t &ast,
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
