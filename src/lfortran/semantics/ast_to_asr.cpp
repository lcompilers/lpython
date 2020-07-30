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

class SymbolTableVisitor : public AST::BaseWalkVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTable *translation_unit_scope;
    SymbolTable *current_scope;

    SymbolTableVisitor(Allocator &al) : al{al} {
        translation_unit_scope = al.make_new<SymbolTable>();
    }

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
        asr = ASR::make_TranslationUnit_t(al, x.base.base.loc,
            translation_unit_scope, nullptr, 0);
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
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
        if (translation_unit_scope->scope.find(sym_name) != translation_unit_scope->scope.end()) {
            throw SemanticError("Subroutine already defined", asr->loc);
        }
        translation_unit_scope->scope[sym_name] = asr;
    }

    void visit_Function(const AST::Function_t &x) {
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
        if (translation_unit_scope->scope.find(sym_name) != translation_unit_scope->scope.end()) {
            throw SemanticError("Function already defined", asr->loc);
        }
        translation_unit_scope->scope[sym_name] = asr;
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
};

class BodyVisitor : public AST::BaseVisitor<BodyVisitor>
{
public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    SymbolTable *current_scope;
    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        current_scope = TRANSLATION_UNIT(asr)->m_global_scope;
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
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
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
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
        LFORTRAN_ASSERT(left->type == right->type);
        // TODO: For now assume reals:
        ASR::ttype_t *type = TYPE(ASR::make_Real_t(al, x.base.base.loc,
                4, nullptr, 0));
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
