#include <iostream>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/parser/parser_stype.h>


namespace LFortran {

static inline ASR::expr_t* EXPR(const ASR::asr_t *f)
{
    LFORTRAN_ASSERT(f->type == ASR::asrType::expr);
    return (ASR::expr_t*)f;
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

class SymbolTableVisitor : public AST::BaseVisitor<SymbolTableVisitor>
{
public:
    ASR::asr_t *asr;
    Allocator &al;
    SymbolTableVisitor(Allocator &al) : al{al} {}

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    }

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

    void visit_TranslationUnit(const AST::TranslationUnit_t &x) {
        for (size_t i=0; i<x.n_items; i++) {
            visit_ast(*x.m_items[i]);
        }
    }

    void visit_Subroutine(const AST::Subroutine_t &x) {
    }

    void visit_Function(const AST::Function_t &x) {
        Vec<ASR::stmt_t*> body;
        body.reserve(al, 8);
        for (size_t i=0; i<x.n_body; i++) {
            this->visit_stmt(*x.m_body[i]);
            ASR::stmt_t *stmt = STMT(tmp);
            body.push_back(al, stmt);
        }
        // TODO:
        // We must keep track of the current scope, lookup this function in the
        // scope as "_current_function" and attach the body to it. For now we
        // simply assume `asr` is this very function:
        ASR::Function_t *current_fn = (ASR::Function_t*)asr;
        current_fn->m_body = &body.p[0];
        current_fn->n_body = body.size();
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
