#include <iostream>
#include <map>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>


namespace LFortran {


class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTable *current_scope;

    SymbolTableVisitor(Allocator &al, SymbolTable *symbol_table)
        : al{al}, current_scope{symbol_table} { }

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        if (!current_scope) {
            current_scope = al.make_new<SymbolTable>(nullptr);
        }
        LFORTRAN_ASSERT(current_scope != nullptr);
        for (size_t i=0; i<x.n_items; i++) {
            AST::astType t = x.m_items[i]->type;
            if (t != AST::astType::expr && t != AST::astType::stmt) {
                visit_ast(*x.m_items[i]);
            }
        }
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_use; i++) {
            visit_unit_decl1(*x.m_use[i]);
        }
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }
        asr = ASR::make_Program_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_body */ nullptr,
            /* n_body */ 0);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Program already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = asr;
        current_scope = parent_scope;
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::asr_t *arg_asr = current_scope->scope[arg_s];
            ASR::var_t *var = VAR(arg_asr);
            args.push_back(al, EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }
        asr = ASR::make_Subroutine_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_bind */ nullptr);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Subroutine already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = asr;
        current_scope = parent_scope;
    }

    void visit_Function(const AST::Function_t &x) {
        // Extract local (including dummy) variables first
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>(parent_scope);
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }

        // Convert and check arguments
        Vec<ASR::expr_t*> args;
        args.reserve(al, x.n_args);
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string arg_s = arg;
            if (current_scope->scope.find(arg_s) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + arg_s + "' not defined", x.base.base.loc);
            }
            ASR::asr_t *arg_asr = current_scope->scope[arg_s];
            ASR::var_t *var = VAR(arg_asr);
            args.push_back(al, EXPR(ASR::make_Var_t(al, x.base.base.loc,
                var)));
        }

        // Handle the return variable and type
        // First determine the name of the variable: either the function name
        // or result(...)
        char *return_var_name;
        if (x.m_return_var) {
            if (x.m_return_var->type == AST::exprType::Name) {
                return_var_name = ((AST::Name_t*)(x.m_return_var))->m_id;
            } else {
                throw SemanticError("Return variable must be an identifier",
                    x.m_return_var->base.loc);
            }
        } else {
            return_var_name = x.m_name;
        }

        // Determine the type of the variable, the type is either specified as
        //     integer function f()
        // or in local variables as
        //     integer :: f
        ASR::asr_t *return_var;
        if (current_scope->scope.find(std::string(return_var_name)) == current_scope->scope.end()) {
            // The variable is not defined among local variables, extract the
            // type from "integer function f()" and add the variable.
            ASR::ttype_t *type;
            if (!x.m_return_type) {
                throw SemanticError("Return type not specified",
                        x.base.base.loc);
            }
            std::string return_type = x.m_return_type;
            if (return_type == "integer") {
                type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
            } else if (return_type == "real") {
                type = TYPE(ASR::make_Real_t(al, x.base.base.loc, 4, nullptr, 0));
            } else if (return_type == "logical") {
                type = TYPE(ASR::make_Logical_t(al, x.base.base.loc, 4, nullptr, 0));
            } else {
                throw SemanticError("Return type not supported",
                        x.base.base.loc);
            }
            // Add it as a local variable:
            return_var = ASR::make_Variable_t(al, x.base.base.loc,
                current_scope, return_var_name, intent_return_var, type);
            current_scope->scope[std::string(return_var_name)] = return_var;
        } else {
            if (x.m_return_type) {
                throw SemanticError("Cannot specify the return type twice",
                    x.base.base.loc);
            }
            // Extract the variable from the local scope
            return_var = current_scope->scope[std::string(return_var_name)];
            VARIABLE(return_var)->m_intent = intent_return_var;
        }

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
            VAR(return_var));

        // Create and register the function
        asr = ASR::make_Function_t(
            al, x.base.base.loc,
            /* a_symtab */ current_scope,
            /* a_name */ x.m_name,
            /* a_args */ args.p,
            /* n_args */ args.size(),
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_bind */ nullptr,
            /* a_return_var */ EXPR(return_var_ref),
            /* a_module */ nullptr);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Function already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = asr;
        current_scope = parent_scope;
    }

    void visit_Declaration(const AST::Declaration_t &x) {
        for (size_t i=0; i<x.n_vars; i++) {
            this->visit_decl(x.m_vars[i]);
        }
    }

    void visit_decl(const AST::decl_t &x) {
        std::string sym = x.m_sym;
        std::string sym_type = x.m_sym_type;
        Location loc;
        // TODO: decl_t does not have location information...
        loc.first_column = 0;
        loc.first_line = 0;
        loc.last_column = 0;
        loc.last_line = 0;
        if (current_scope->scope.find(sym) == current_scope->scope.end()) {
            ASR::intentType s_intent=intent_local;
            Vec<ASR::dimension_t> dims;
            dims.reserve(al, x.n_dims);
            if (x.n_attrs > 0) {
                AST::Attribute_t *a = (AST::Attribute_t*)(x.m_attrs[0]);
                if (std::string(a->m_name) == "intent") {
                    if (a->n_args > 0) {
                        std::string intent = std::string(a->m_args[0].m_arg);
                        if (intent == "in") {
                            s_intent = intent_in;
                        } else if (intent == "out") {
                            s_intent = intent_out;
                        } else if (intent == "inout") {
                            s_intent = intent_inout;
                        } else {
                            throw SemanticError("Incorrect intent specifier", loc);
                        }
                    } else {
                        throw SemanticError("intent() is empty. Must specify intent", loc);
                    }
                }
                if (std::string(a->m_name) == "dimension") {
                    if (x.n_dims > 0) {
                        throw SemanticError("Cannot specify dimensions both ways", loc);
                    }
                    dims.reserve(al, a->n_dims);
                    for (size_t i=0; i<a->n_dims; i++) {
                        ASR::dimension_t dim;
                        if (a->m_dims[i].m_start) {
                            this->visit_expr(*a->m_dims[i].m_start);
                            dim.m_start = EXPR(asr);
                        } else {
                            dim.m_start = nullptr;
                        }
                        if (a->m_dims[i].m_end) {
                            this->visit_expr(*a->m_dims[i].m_end);
                            dim.m_end = EXPR(asr);
                        } else {
                            dim.m_end = nullptr;
                        }
                        dims.push_back(al, dim);
                    }
                }
            }
            for (size_t i=0; i<x.n_dims; i++) {
                ASR::dimension_t dim;
                if (x.m_dims[i].m_start) {
                    this->visit_expr(*x.m_dims[i].m_start);
                    dim.m_start = EXPR(asr);
                } else {
                    dim.m_start = nullptr;
                }
                if (x.m_dims[i].m_end) {
                    this->visit_expr(*x.m_dims[i].m_end);
                    dim.m_end = EXPR(asr);
                } else {
                    dim.m_end = nullptr;
                }
                dims.push_back(al, dim);
            }
            ASR::ttype_t *type;
            if (sym_type == "real") {
                type = TYPE(ASR::make_Real_t(al, loc, 4, dims.p, dims.size()));
            } else if (sym_type == "integer") {
                type = TYPE(ASR::make_Integer_t(al, loc, 4, dims.p, dims.size()));
            } else if (sym_type == "logical") {
                type = TYPE(ASR::make_Logical_t(al, loc, 4, dims.p, dims.size()));
            } else {
                throw SemanticError("Unsupported type", loc);
            }
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, current_scope,
                x.m_sym, s_intent, type);
            current_scope->scope[sym] = v;

        }
    }

    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        asr = ASR::make_Num_t(al, x.base.base.loc, x.m_n, type);
    }

};

class BodyVisitor : public AST::BaseVisitor<BodyVisitor>
{
public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    SymbolTable *current_scope;
    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        ASR::TranslationUnit_t *unit = TRANSLATION_UNIT(asr);
        current_scope = unit->m_global_scope;
        Vec<ASR::asr_t*> items;
        items.reserve(al, x.n_items);
        for (size_t i=0; i<x.n_items; i++) {
            tmp = nullptr;
            visit_ast(*x.m_items[i]);
            if (tmp) {
                items.push_back(al, tmp);
            }
        }
        unit->m_items = items.p;
        unit->n_items = items.size();
    }

    void visit_Declaration(const AST::Declaration_t & /* x */) {
        // This AST node was already visited in SymbolTableVisitor
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::asr_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Program_t *v = PROGRAM(t);
        current_scope = v->m_symtab;

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            ASR::stmt_t *stmt = STMT(tmp);
            body.push_back(al, stmt);
        }
        v->m_body = body.p;
        v->n_body = body.size();

        for (size_t i=0; i<x.n_contains; i++) {
            visit_program_unit(*x.m_contains[i]);
        }

        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    // TODO: add SymbolTable::lookup_symbol(), which will automatically return
    // an error
    // TODO: add SymbolTable::get_symbol(), which will only check in Debug mode
        SymbolTable *old_scope = current_scope;
        ASR::asr_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Subroutine_t *v = SUBROUTINE(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            ASR::stmt_t *stmt = STMT(tmp);
            body.push_back(al, stmt);
        }
        v->m_body = body.p;
        v->n_body = body.size();
        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Function(const AST::Function_t &x) {
        SymbolTable *old_scope = current_scope;
        ASR::asr_t *t = current_scope->scope[std::string(x.m_name)];
        ASR::Function_t *v = FUNCTION(t);
        current_scope = v->m_symtab;
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            ASR::stmt_t *stmt = STMT(tmp);
            body.push_back(al, stmt);
        }
        v->m_body = body.p;
        v->n_body = body.size();
        current_scope = old_scope;
        tmp = nullptr;
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = EXPR(tmp);
        if (target->type == ASR::exprType::Var) {
            // integer :: i
            // i = ...
        } else if (target->type == ASR::exprType::ArrayRef) {
            // integer :: i(5)
            // i(3) = ...
        } else {
            throw SemanticError("The LHS of assignment can only be a variable or an array reference",
                x.base.base.loc);
        }
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        if (target->type == ASR::exprType::Var) {
            // integer :: i
            // i = ...
            if (expr_type(target)->type == ASR::ttypeType::Real) {
                if (expr_type(value)->type == ASR::ttypeType::Real) {
                    // TODO: convert/cast kinds if they differ
                } else if (expr_type(value)->type == ASR::ttypeType::Integer) {
                    value = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                        value, ASR::cast_kindType::IntegerToReal, expr_type(target));
                } else {
                    throw SemanticError("Only Integer or Real can be assigned to Real",
                        x.base.base.loc);
                }
            } else if (expr_type(target)->type == ASR::ttypeType::Integer) {
                if (expr_type(value)->type == ASR::ttypeType::Real) {
                    value = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                        value, ASR::cast_kindType::RealToInteger, expr_type(target));
                } else if (expr_type(value)->type == ASR::ttypeType::Integer) {
                    // TODO: convert/cast kinds if they differ
                } else {
                    throw SemanticError("Only Integer or Real can be assigned to Integer",
                        x.base.base.loc);
                }
            }
        }
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }

    Vec<ASR::expr_t*> visit_expr_list(AST::expr_t **ast_list, size_t n) {
        Vec<ASR::expr_t*> asr_list;
        asr_list.reserve(al, n);
        for (size_t i=0; i<n; i++) {
            visit_expr(*ast_list[i]);
            ASR::expr_t *expr = EXPR(tmp);
            asr_list.push_back(al, expr);
        }
        return asr_list;
    }

    void visit_SubroutineCall(const AST::SubroutineCall_t &x) {
        ASR::Subroutine_t *sub = resolve_subroutine(x.base.base.loc, x.m_name);
        Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
        tmp = ASR::make_SubroutineCall_t(al, x.base.base.loc,
                (ASR::sub_t*)sub, args.p, args.size());
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        // Cast LHS or RHS if necessary
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        if (left_type->type == ASR::ttypeType::Real) {
            if (right_type->type == ASR::ttypeType::Real) {
                // TODO: convert/cast kinds if they differ
            } else if (right_type->type == ASR::ttypeType::Integer) {
                right = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                    right, ASR::cast_kindType::IntegerToReal, left_type);
            } else {
                throw SemanticError("Compare: only Integer or Real can be on the RHS with Real as LHS",
                    x.base.base.loc);
            }
        } else if (left_type->type == ASR::ttypeType::Integer) {
            if (right_type->type == ASR::ttypeType::Real) {
                left = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                    left, ASR::cast_kindType::IntegerToReal, right_type);
            } else if (right_type->type == ASR::ttypeType::Integer) {
                // TODO: convert/cast kinds if they differ
            } else {
                throw SemanticError("Compare: Only Integer or Real can be on the RHS with Integer as LHS",
                    x.base.base.loc);
            }
        }
        LFORTRAN_ASSERT(expr_type(left)->type == expr_type(right)->type);
        ASR::ttype_t *type = TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        ASR::cmpopType asr_op;
        switch (x.m_op) {
            case (AST::cmpopType::Eq) : { asr_op = ASR::cmpopType::Eq; break;}
            case (AST::cmpopType::Gt) : { asr_op = ASR::cmpopType::Gt; break;}
            case (AST::cmpopType::GtE) : { asr_op = ASR::cmpopType::GtE; break;}
            case (AST::cmpopType::Lt) : { asr_op = ASR::cmpopType::Lt; break;}
            case (AST::cmpopType::LtE) : { asr_op = ASR::cmpopType::LtE; break;}
            case (AST::cmpopType::NotEq) : { asr_op = ASR::cmpopType::NotEq; break;}
            default : {
                throw SemanticError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
        tmp = ASR::make_Compare_t(al, x.base.base.loc,
            left, asr_op, right, type);
    }

    void visit_BinOp(const AST::BinOp_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        ASR::operatorType op;
        switch (x.m_op) {
            case (AST::Add) :
                op = ASR::Add;
                break;
            case (AST::Sub) :
                op = ASR::Sub;
                break;
            case (AST::Mul) :
                op = ASR::Mul;
                break;
            case (AST::Div) :
                op = ASR::Div;
                break;
            case (AST::Pow) :
                op = ASR::Pow;
                break;
        }
        // Cast LHS or RHS if necessary
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        ASR::ttype_t *type;
        if (left_type->type == ASR::ttypeType::Real) {
            if (right_type->type == ASR::ttypeType::Real) {
                // TODO: convert/cast kinds if they differ
                type = left_type;
            } else if (right_type->type == ASR::ttypeType::Integer) {
                right = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                    right, ASR::cast_kindType::IntegerToReal, left_type);
                type = left_type;
            } else {
                throw SemanticError("Binop: only Integer or Real can be on the RHS with Real as LHS",
                    x.base.base.loc);
            }
        } else if (left_type->type == ASR::ttypeType::Integer) {
            if (right_type->type == ASR::ttypeType::Real) {
                left = (ASR::expr_t*)ASR::make_ImplicitCast_t(al, x.base.base.loc,
                    left, ASR::cast_kindType::IntegerToReal, right_type);
                type = right_type;
            } else if (right_type->type == ASR::ttypeType::Integer) {
                // TODO: convert/cast kinds if they differ
                type = left_type;
            } else {
                throw SemanticError("BinOp: Only Integer or Real can be on the RHS with Integer as LHS",
                    x.base.base.loc);
            }
        }
        LFORTRAN_ASSERT(expr_type(left)->type == expr_type(right)->type);
        tmp = ASR::make_BinOp_t(al, x.base.base.loc,
                left, op, right, type);
    }

    void visit_UnaryOp(const AST::UnaryOp_t &x) {
        this->visit_expr(*x.m_operand);
        ASR::expr_t *operand = EXPR(tmp);
        ASR::unaryopType op;
        switch (x.m_op) {
            case (AST::unaryopType::Invert) :
                op = ASR::unaryopType::Invert;
                break;
            case (AST::unaryopType::Not) :
                op = ASR::unaryopType::Not;
                break;
            case (AST::unaryopType::UAdd) :
                op = ASR::unaryopType::UAdd;
                break;
            case (AST::unaryopType::USub) :
                op = ASR::unaryopType::USub;
                break;
        }
        ASR::ttype_t *operand_type = expr_type(operand);
        tmp = ASR::make_UnaryOp_t(al, x.base.base.loc,
                op, operand, operand_type);
    }

    ASR::asr_t* resolve_variable(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string var_name = id;
        ASR::asr_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            throw SemanticError("Variable '" + var_name + "' not declared", loc);
        }
        ASR::var_t *var = VAR(v);
        return ASR::make_Var_t(al, loc, var);
    }

    ASR::Subroutine_t* resolve_subroutine(const Location &loc, const char* id) {
        SymbolTable *scope = current_scope;
        std::string sub_name = id;
        ASR::asr_t *sub = scope->resolve_symbol(sub_name);
        if (!sub) {
            throw SemanticError("Subroutine '" + sub_name + "' not declared", loc);
        }
        return SUBROUTINE(sub);
    }

    void visit_Name(const AST::Name_t &x) {
        tmp = resolve_variable(x.base.base.loc, x.m_id);
    }

    void visit_FuncCallOrArray(const AST::FuncCallOrArray_t &x) {
        SymbolTable *scope = current_scope;
        std::string var_name = x.m_func;
        ASR::asr_t *v = scope->resolve_symbol(var_name);
        if (!v) {
            if (var_name == "size") {
                // Intrinsic function size(), add it to the global scope
                ASR::TranslationUnit_t *unit = (ASR::TranslationUnit_t*)asr;
                const char* fn_name_orig = "size";
                char *fn_name = (char*)fn_name_orig;
                SymbolTable *fn_scope = al.make_new<SymbolTable>(unit->m_global_scope);
                ASR::ttype_t *type;
                Location loc;
                type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
                ASR::asr_t *return_var = ASR::make_Variable_t(al, loc,
                    fn_scope, fn_name, intent_return_var, type);
                fn_scope->scope[std::string(fn_name)] = return_var;
                ASR::asr_t *return_var_ref = ASR::make_Var_t(al, loc,
                    VAR(return_var));
                ASR::asr_t *fn = ASR::make_Function_t(
                    al, loc,
                    /* a_symtab */ fn_scope,
                    /* a_name */ fn_name,
                    /* a_args */ nullptr,
                    /* n_args */ 0,
                    /* a_body */ nullptr,
                    /* n_body */ 0,
                    /* a_bind */ nullptr,
                    /* a_return_var */ EXPR(return_var_ref),
                    /* a_module */ nullptr);
                std::string sym_name = fn_name;
                unit->m_global_scope->scope[sym_name] = fn;
                v = fn;
            } else {
                throw SemanticError("Function or array '" + var_name
                    + "' not declared", x.base.base.loc);
            }
        }
        switch (v->type) {
            case (ASR::asrType::fn) : {
                Vec<ASR::expr_t*> args = visit_expr_list(x.m_args, x.n_args);
                ASR::ttype_t *type;
                type = VARIABLE((ASR::asr_t*)(EXPR_VAR((ASR::asr_t*)(FUNCTION(v)->m_return_var))->m_v))->m_type;
                tmp = ASR::make_FuncCall_t(al, x.base.base.loc,
                    (ASR::fn_t*)v, args.p, args.size(), nullptr, 0, type);
                break;
            }
            case (ASR::asrType::var) : {
                Vec<ASR::array_index_t> args;
                args.reserve(al, x.n_args);
                for (size_t i=0; i<x.n_args; i++) {
                    visit_expr(*x.m_args[i]);
                    ASR::array_index_t ai;
                    ai.m_left = nullptr;
                    ai.m_right = EXPR(tmp);
                    ai.m_step = nullptr;
                    args.push_back(al, ai);
                }

                ASR::ttype_t *type;
                type = VARIABLE(v)->m_type;
                tmp = ASR::make_ArrayRef_t(al, x.base.base.loc,
                    (ASR::var_t*)v, args.p, args.size(), type);
                break;
            }
            default : throw SemanticError("Symbol '" + var_name
                    + "' is not a function or an array", x.base.base.loc);
        }
    }

    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_Num_t(al, x.base.base.loc, x.m_n, type);
    }

    void visit_Constant(const AST::Constant_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Logical_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_Constant_t(al, x.base.base.loc, x.m_value, type);
    }

    void visit_Str(const AST::Str_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Character_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Str_t(al, x.base.base.loc, x.m_s, type);
    }

    void visit_Real(const AST::Real_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                4, nullptr, 0));
        tmp = ASR::make_ConstantReal_t(al, x.base.base.loc, x.m_n, type);
    }

    void visit_Print(const AST::Print_t &x) {
        Vec<ASR::expr_t*> body;
        body.reserve(al, x.n_values);
        for (size_t i=0; i<x.n_values; i++) {
            visit_expr(*x.m_values[i]);
            ASR::expr_t *expr = EXPR(tmp);
            body.push_back(al, expr);
        }
        tmp = ASR::make_Print_t(al, x.base.base.loc, nullptr,
            body.p, body.size());
    }

    void visit_If(const AST::If_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            body.push_back(al, STMT(tmp));
        }
        Vec<ASR::stmt_t*> orelse;
        orelse.reserve(al, x.n_orelse);
        for (size_t i=0; i<x.n_orelse; i++) {
            visit_stmt(*x.m_orelse[i]);
            orelse.push_back(al, STMT(tmp));
        }
        tmp = ASR::make_If_t(al, x.base.base.loc, test, body.p,
                body.size(), orelse.p, orelse.size());
    }

    void visit_WhileLoop(const AST::WhileLoop_t &x) {
        visit_expr(*x.m_test);
        ASR::expr_t *test = EXPR(tmp);
        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            body.push_back(al, STMT(tmp));
        }
        tmp = ASR::make_WhileLoop_t(al, x.base.base.loc, test, body.p,
                body.size());
    }

    void visit_DoLoop(const AST::DoLoop_t &x) {
        if (! x.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! x.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! x.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = EXPR(resolve_variable(x.base.base.loc, x.m_var));
        visit_expr(*x.m_start);
        ASR::expr_t *start = EXPR(tmp);
        visit_expr(*x.m_end);
        ASR::expr_t *end = EXPR(tmp);
        ASR::expr_t *increment;
        if (x.m_increment) {
            visit_expr(*x.m_increment);
            increment = EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            body.push_back(al, STMT(tmp));
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        tmp = ASR::make_DoLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_DoConcurrentLoop(const AST::DoConcurrentLoop_t &x) {
        if (! x.m_var) {
            throw SemanticError("Do loop: loop variable is required for now",
                x.base.base.loc);
        }
        if (! x.m_start) {
            throw SemanticError("Do loop: start condition required for now",
                x.base.base.loc);
        }
        if (! x.m_end) {
            throw SemanticError("Do loop: end condition required for now",
                x.base.base.loc);
        }
        ASR::expr_t *var = EXPR(resolve_variable(x.base.base.loc, x.m_var));
        visit_expr(*x.m_start);
        ASR::expr_t *start = EXPR(tmp);
        visit_expr(*x.m_end);
        ASR::expr_t *end = EXPR(tmp);
        ASR::expr_t *increment;
        if (x.m_increment) {
            visit_expr(*x.m_increment);
            increment = EXPR(tmp);
        } else {
            increment = nullptr;
        }

        Vec<ASR::stmt_t*> body;
        body.reserve(al, x.n_body);
        for (size_t i=0; i<x.n_body; i++) {
            visit_stmt(*x.m_body[i]);
            body.push_back(al, STMT(tmp));
        }
        ASR::do_loop_head_t head;
        head.m_v = var;
        head.m_start = start;
        head.m_end = end;
        head.m_increment = increment;
        tmp = ASR::make_DoConcurrentLoop_t(al, x.base.base.loc, head, body.p,
                body.size());
    }

    void visit_Exit(const AST::Exit_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Exit_t(al, x.base.base.loc);
    }

    void visit_Cycle(const AST::Cycle_t &x) {
        // TODO: add a check here that we are inside a While loop
        tmp = ASR::make_Cycle_t(al, x.base.base.loc);
    }

    void visit_Stop(const AST::Stop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_Stop_t(al, x.base.base.loc, code);
    }

    void visit_ErrorStop(const AST::ErrorStop_t &x) {
        ASR::expr_t *code;
        if (x.m_code) {
            visit_expr(*x.m_code);
            code = EXPR(tmp);
        } else {
            code = nullptr;
        }
        tmp = ASR::make_ErrorStop_t(al, x.base.base.loc, code);
    }
};

ASR::TranslationUnit_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast,
        SymbolTable *symbol_table)
{
    SymbolTableVisitor v(al, symbol_table);
    v.visit_TranslationUnit(ast);
    ASR::asr_t *unit = v.asr;

    // Uncomment for debugging the ASR after SymbolTable building:
    // std::cout << pickle(*unit) << std::endl;

    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    return TRANSLATION_UNIT(unit);
}

} // namespace LFortran
