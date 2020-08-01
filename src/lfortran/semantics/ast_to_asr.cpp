#include <iostream>
#include <map>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/pickle.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>


namespace LFortran {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
}

static inline ASR::var_t* VAR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    return (ASR::var_t*)f;
}

static inline ASR::Variable_t* VARIABLE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::var);
    ASR::var_t *t = (ASR::var_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::varType::Variable);
    return (ASR::Variable_t*)t;
}

static inline ASR::Subroutine_t* SUBROUTINE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::sub);
    ASR::sub_t *t = (ASR::sub_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::subType::Subroutine);
    return (ASR::Subroutine_t*)t;
}

static inline ASR::Function_t* FUNCTION(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::fn);
    ASR::fn_t *t = (ASR::fn_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::fnType::Function);
    return (ASR::Function_t*)t;
}

static inline ASR::Program_t* PROGRAM(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::prog);
    ASR::prog_t *t = (ASR::prog_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::progType::Program);
    return (ASR::Program_t*)t;
}

static inline ASR::TranslationUnit_t* TRANSLATION_UNIT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::unit);
    ASR::unit_t *t = (ASR::unit_t *)f;
    LFORTRAN_ASSERT(t->type == ASR::unitType::TranslationUnit);
    return (ASR::TranslationUnit_t*)t;
}

static inline ASR::stmt_t* STMT(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::stmt);
    return (ASR::stmt_t*)f;
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    return (ASR::ttype_t*)f;
}

static inline ASR::ttype_t* expr_type(const ASR::expr_t *f)
{
    switch (f->type) {
        case ASR::exprType::BoolOp: { return ((ASR::BoolOp_t*)f)->m_type; }
        case ASR::exprType::BinOp: { return ((ASR::BinOp_t*)f)->m_type; }
        case ASR::exprType::UnaryOp: { return ((ASR::UnaryOp_t*)f)->m_type; }
        case ASR::exprType::Compare: { return ((ASR::Compare_t*)f)->m_type; }
        case ASR::exprType::FuncCall: { return ((ASR::FuncCall_t*)f)->m_type; }
        case ASR::exprType::ArrayRef: { return ((ASR::ArrayRef_t*)f)->m_type; }
        case ASR::exprType::ArrayInitializer: { return ((ASR::ArrayInitializer_t*)f)->m_type; }
        case ASR::exprType::Num: { return ((ASR::Num_t*)f)->m_type; }
        case ASR::exprType::Str: { return ((ASR::Str_t*)f)->m_type; }
        case ASR::exprType::VariableOld: { return ((ASR::VariableOld_t*)f)->m_type; }
        case ASR::exprType::Var: { return VARIABLE((ASR::asr_t*)((ASR::Var_t*)f)->m_v)->m_type; }
        case ASR::exprType::Constant: { return ((ASR::Constant_t*)f)->m_type; }
        default : throw LFortranException("Not implemented");
    }
}

class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTable *current_scope;

    SymbolTableVisitor(Allocator &al) : al{al}, current_scope{nullptr} { }

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        LFORTRAN_ASSERT(current_scope == nullptr);
        current_scope = al.make_new<SymbolTable>();
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            current_scope, nullptr, 0);
    }

    void visit_Program(const AST::Program_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>();
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
            /* a_name */ x.m_name,
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_symtab */ current_scope);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Program already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = asr;
        current_scope = parent_scope;
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>();
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        // TODO: save the arguments into `a_args` and `n_args`.
        // We need to get Variables settled first, then it will be just a
        // reference to a variable.
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string args = arg;
            if (current_scope->scope.find(args) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + args + "' not defined", x.base.base.loc);
            }
        }
        asr = ASR::make_Subroutine_t(
            al, x.base.base.loc,
            /* a_name */ x.m_name,
            /* a_args */ nullptr,
            /* n_args */ 0,
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_bind */ nullptr,
            /* a_symtab */ current_scope);
        std::string sym_name = x.m_name;
        if (parent_scope->scope.find(sym_name) != parent_scope->scope.end()) {
            throw SemanticError("Subroutine already defined", asr->loc);
        }
        parent_scope->scope[sym_name] = asr;
        current_scope = parent_scope;
    }

    void visit_Function(const AST::Function_t &x) {
        SymbolTable *parent_scope = current_scope;
        current_scope = al.make_new<SymbolTable>();
        for (size_t i=0; i<x.n_decl; i++) {
            visit_unit_decl2(*x.m_decl[i]);
        }
        // TODO: save the arguments into `a_args` and `n_args`.
        // We need to get Variables settled first, then it will be just a
        // reference to a variable.
        for (size_t i=0; i<x.n_args; i++) {
            char *arg=x.m_args[i].m_arg;
            std::string args = arg;
            if (current_scope->scope.find(args) == current_scope->scope.end()) {
                throw SemanticError("Dummy argument '" + args + "' not defined", x.base.base.loc);
            }
        }
        ASR::ttype_t *type;
        type = TYPE(ASR::make_Integer_t(al, x.base.base.loc, 4, nullptr, 0));
        ASR::asr_t *return_var = ASR::make_Variable_t(al, x.base.base.loc,
            x.m_name, intent_return_var, type);
        current_scope->scope[std::string(x.m_name)] = return_var;

        ASR::asr_t *return_var_ref = ASR::make_Var_t(al, x.base.base.loc,
            current_scope, VAR(return_var));

        asr = ASR::make_Function_t(
            al, x.base.base.loc,
            /* a_name */ x.m_name,
            /* a_args */ nullptr,
            /* n_args */ 0,
            /* a_body */ nullptr,
            /* n_body */ 0,
            /* a_bind */ nullptr,
            /* a_return_var */ EXPR(return_var_ref),
            /* a_module */ nullptr,
            /* a_symtab */ current_scope);
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
        if (current_scope->scope.find(sym) == current_scope->scope.end()) {
            int s_type;
            if (sym_type == "integer") {
                s_type = 2;
            } else if (sym_type == "real") {
                s_type = 1;
            } else {
                Location loc;
                // TODO: decl_t does not have location information...
                loc.first_column = 0;
                loc.first_line = 0;
                loc.last_column = 0;
                loc.last_line = 0;
                throw SemanticError("Unsupported type", loc);
            }
            int s_intent=intent_local;
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
                            Location loc;
                            // TODO: decl_t does not have location information...
                            loc.first_column = 0;
                            loc.first_line = 0;
                            loc.last_column = 0;
                            loc.last_line = 0;
                            throw SemanticError("Incorrect intent specifier", loc);
                        }
                    } else {
                        Location loc;
                        // TODO: decl_t does not have location information...
                        loc.first_column = 0;
                        loc.first_line = 0;
                        loc.last_column = 0;
                        loc.last_line = 0;
                        throw SemanticError("intent() is empty. Must specify intent", loc);
                    }
                }
            }
            Location loc;
            // TODO: decl_t does not have location information...
            loc.first_column = 0;
            loc.first_line = 0;
            loc.last_column = 0;
            loc.last_line = 0;
            ASR::ttype_t *type;
            if (s_type == 1) {
                type = TYPE(ASR::make_Real_t(al, loc, 4, nullptr, 0));
            } else {
                LFORTRAN_ASSERT(s_type == 2);
                type = TYPE(ASR::make_Integer_t(al, loc, 4, nullptr, 0));
            }
            ASR::asr_t *v = ASR::make_Variable_t(al, loc, x.m_sym, s_intent, type);
            current_scope->scope[sym] = v;

        }
    }

    void visit_expr(const AST::expr_t &x) {}
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
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }

    void visit_Compare(const AST::Compare_t &x) {
        this->visit_expr(*x.m_left);
        ASR::expr_t *left = EXPR(tmp);
        this->visit_expr(*x.m_right);
        ASR::expr_t *right = EXPR(tmp);
        // TODO: For now we require the types to match (implicit casting is not
        // implemented yet)
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        LFORTRAN_ASSERT(left_type->type == right_type->type);
        ASR::ttype_t *type = left_type;
        switch (x.m_op) {
            case (AST::cmpopType::Eq) : {
                tmp = ASR::make_Compare_t(al, x.base.base.loc,
                    left, ASR::cmpopType::Eq, right, type);
                break;
            }
            default : {
                throw SemanticError("Comparison operator not implemented",
                        x.base.base.loc);
            }
        }
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
        // TODO: For now we require the types to match (implicit casting is not
        // implemented yet)
        ASR::ttype_t *left_type = expr_type(left);
        ASR::ttype_t *right_type = expr_type(right);
        LFORTRAN_ASSERT(left_type->type == right_type->type);
        ASR::ttype_t *type = left_type;
        tmp = ASR::make_BinOp_t(al, x.base.base.loc,
                left, op, right, type);
    }

    void visit_Name(const AST::Name_t &x) {
        SymbolTable *scope = current_scope;
        std::string var_name = x.m_id;
        if (scope->scope.find(var_name) == scope->scope.end()) {
            throw SemanticError("Variable '" + var_name + "' not declared", x.base.base.loc);
        }
        ASR::Variable_t *v = VARIABLE(scope->scope[std::string(var_name)]);
        ASR::var_t *var = (ASR::var_t*)v;
        ASR::asr_t *tmp2 = ASR::make_Var_t(al, x.base.base.loc, scope, var);
        tmp = tmp2;
        /*
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_VariableOld_t(al, x.base.base.loc,
                x.m_id, nullptr, 1, type);
        */
    }

    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Num_t(al, x.base.base.loc, x.m_n, type);
    }

    void visit_Real(const AST::Real_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                4, nullptr, 0));
        std::string f = x.m_n;
        float f2 = std::stof(f);
        // TODO: represent Real numbers properly in ASR
        int f3 = int(f2); // For now we cast floats to ints
        tmp = ASR::make_Num_t(al, x.base.base.loc, f3, type);
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

ASR::asr_t *ast_to_asr(Allocator &al, AST::TranslationUnit_t &ast)
{
    SymbolTableVisitor v(al);
    v.visit_TranslationUnit(ast);
    ASR::asr_t *unit = v.asr;

    BodyVisitor b(al, unit);
    b.visit_TranslationUnit(ast);
    return unit;
}

} // namespace LFortran
