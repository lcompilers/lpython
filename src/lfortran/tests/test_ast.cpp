#include <tests/doctest.h>

#include <iostream>

#include <lfortran/parser/alloc.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>

namespace LFortran {

TEST_CASE("Operator types") {
    std::cout << "OK: " << AST::operatorType::Pow  << std::endl;
    std::cout << "OK: " << ASR::operatorType::Pow  << std::endl;
}


TEST_CASE("Test types") {
    Allocator al(1024*1024);
    Location loc;

    AST::ast_t &a = *AST::make_Num_t(al, loc, 5);
    CHECK(AST::is_a<AST::expr_t>(a));
    CHECK(! AST::is_a<AST::stmt_t>(a));

    AST::Num_t &x = *AST::down_cast4<AST::Num_t>(&a);
    CHECK(x.m_n == 5);

}

} // namespace LFortran
