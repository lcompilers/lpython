#include <iostream>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/semantics/ast_to_asr.h>


namespace LFortran {

class SymbolTableVisitor : public LFortran::AST::BaseVisitor<SymbolTableVisitor>
{
public:
    LFortran::ASR::asr_t *asr;
    Allocator &al;
    SymbolTableVisitor(Allocator &al) : al{al} {}
    void visit_Function(const LFortran::AST::Function_t &x) {
        std::cout << "Function: " << x.m_name << std::endl;
        asr = LFortran::ASR::make_Function_t(al, x.base.base.loc,
            /*char* a_name*/ x.m_name,
            /*expr_t** a_args*/ nullptr, /*size_t n_args*/ 0,
            /*stmt_t** a_body*/ nullptr, /*size_t n_body*/ 0,
            /*tbind_t* a_bind*/ nullptr,
            /*expr_t* a_return_var*/ nullptr,
            /*char* a_module*/ nullptr,
            /*int *object* a_symtab*/ 0);
    }
};

void ast_to_asr(LFortran::AST::ast_t &ast, Allocator &al,
        LFortran::ASR::asr_t **asr)
{
    SymbolTableVisitor v(al);
    v.visit_ast(ast);
    *asr = v.asr;
}

} // namespace LFortran
