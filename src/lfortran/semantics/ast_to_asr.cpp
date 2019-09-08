#include <iostream>
#include <memory>

#include <lfortran/ast.h>
#include <lfortran/semantics/ast_to_asr.h>

using LFortran::AST::BaseVisitor;
using LFortran::AST::TranslationUnit_t;
using LFortran::AST::Function_t;

namespace LFortran {

class SymbolTableVisitor : public BaseVisitor<SymbolTableVisitor>
{
public:
    SymbolTableVisitor() {  }
    void visit_Function(const Function_t &x) {
        std::cout << "Function: " << x.m_name << std::endl;
    }
};

void ast_to_asr(LFortran::AST::ast_t &ast)
{
    SymbolTableVisitor v;
    v.visit_ast(ast);
}

} // namespace LFortran
