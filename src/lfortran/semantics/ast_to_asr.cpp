#include <iostream>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/semantics/ast_to_asr.h>


namespace LFortran {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
}

static inline ASR::ttype_t* TYPE(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::ttype);
    return (ASR::ttype_t*)f;
}

class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTableVisitor(Allocator &al) : al{al} {}
    void visit_Function(const AST::Function_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        ASR::expr_t *return_var = EXPR(ASR::make_Variable_t(al, x.base.base.loc,
                x.m_name, nullptr, 1, type));
        asr = ASR::make_Function_t(al, x.base.base.loc,
            /*char* a_name*/ x.m_name,
            /*expr_t** a_args*/ nullptr, /*size_t n_args*/ 0,
            /*stmt_t** a_body*/ nullptr, /*size_t n_body*/ 0,
            /*tbind_t* a_bind*/ nullptr,
            /*expr_t* a_return_var*/ return_var,
            /*char* a_module*/ nullptr,
            /*int *object* a_symtab*/ 0);
    }
};

class BodyVisitor : public AST::BaseVisitor<BodyVisitor>
{
public:
    Allocator &al;
    ASR::asr_t *asr, *tmp;
    BodyVisitor(Allocator &al, ASR::asr_t *unit) : al{al}, asr{unit} {}

    void visit_Function(const AST::Function_t &x) {
        std::vector<ASR::asr_t*> body;
        for (size_t i=0; i<x.n_body; i++) {
            LFORTRAN_ASSERT(x.m_body[i]->base.type == AST::astType::stmt)
            this->visit_stmt(*x.m_body[i]);
            ASR::asr_t *stmt = tmp;
            body.push_back(stmt);
        }
        // TODO:
        // We must keep track of the current scope, lookup this function in the
        // scope as "_current_function" and attach the body to it:
        // _current_function.m_body = &body[0];
        // _current_function.n_body = body.size();
    }

    void visit_Assignment(const AST::Assignment_t &x) {
        this->visit_expr(*x.m_target);
        ASR::expr_t *target = EXPR(tmp);
        this->visit_expr(*x.m_value);
        ASR::expr_t *value = EXPR(tmp);
        tmp = ASR::make_Assignment_t(al, x.base.base.loc, target, value);
    }
    void visit_Name(const AST::Name_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Variable_t(al, x.base.base.loc,
                x.m_id, nullptr, 1, type);
    }
    void visit_Num(const AST::Num_t &x) {
        ASR::ttype_t *type = TYPE(ASR::make_Integer_t(al, x.base.base.loc,
                8, nullptr, 0));
        tmp = ASR::make_Num_t(al, x.base.base.loc, x.m_n, type);
    }
};

ASR::asr_t *ast_to_asr(Allocator &al, AST::ast_t &ast)
{
    SymbolTableVisitor v(al);
    v.visit_ast(ast);
    ASR::asr_t *unit = v.asr;

    BodyVisitor b(al, unit);
    b.visit_ast(ast);
    return unit;
}

} // namespace LFortran
