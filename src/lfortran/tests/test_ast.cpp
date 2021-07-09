#include <tests/doctest.h>

#include <iostream>

#include <lfortran/parser/alloc.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/asr_verify.h>

namespace LFortran {

TEST_CASE("Operator types") {
    std::cout << "OK: " << AST::operatorType::Pow  << std::endl;
    std::cout << "OK: " << ASR::binopType::Pow  << std::endl;
}


TEST_CASE("Test types") {
    Allocator al(1024*1024);
    Location loc;

    AST::ast_t &a = *AST::make_Num_t(al, loc, 5, nullptr);
    CHECK(AST::is_a<AST::expr_t>(a));
    CHECK(! AST::is_a<AST::stmt_t>(a));

    AST::Num_t &x = *AST::down_cast2<AST::Num_t>(&a);
    CHECK(x.m_n == 5);

}

TEST_CASE("ASR Verify") {
    Allocator al(4*1024);

    std::string src = R"""(
program expr2
implicit none
integer :: x
x = (2+3)*5
print *, x
end program
)""";

    AST::TranslationUnit_t* ast = LFortran::parse2(al, src);
    ASR::TranslationUnit_t* asr = LFortran::ast_to_asr(al, *ast);

    CHECK(asr_verify(*asr)); // Passes

    // Extract the variable "x" from the "x = (2+3)*5" line:
    ASR::Program_t *prog = ASR::down_cast<ASR::Program_t>(asr->m_global_scope->scope["expr2"]);
    ASR::Assignment_t *a = ASR::down_cast<ASR::Assignment_t>(prog->m_body[0]);
    ASR::Var_t *v = ASR::down_cast<ASR::Var_t>(a->m_target);

    v->m_v = &(prog->base); // Assign the wrong symbol to Var_t::m_v

    // This will be caught by the verifier
    CHECK_THROWS_AS(asr_verify(*asr), LFortranException);
}


} // namespace LFortran
