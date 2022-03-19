#include <tests/doctest.h>

#include <iostream>

#include <libasr/alloc.h>
#include <lpython/ast.h>
#include <libasr/asr.h>
#include <libasr/asr_verify.h>

namespace LFortran {


TEST_CASE("Test types") {
    Allocator al(1024*1024);
    Location loc;

    AST::ast_t &a = *AST::make_Num_t(al, loc, 5, nullptr);
    CHECK(AST::is_a<AST::expr_t>(a));
    CHECK(! AST::is_a<AST::stmt_t>(a));

    AST::Num_t &x = *AST::down_cast2<AST::Num_t>(&a);
    CHECK(x.m_n == 5);

}


} // namespace LFortran
