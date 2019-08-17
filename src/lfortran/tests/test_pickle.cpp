#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>

std::string p(Allocator &al, const std::string &s)
{
    LFortran::AST::ast_t* result;
    result = LFortran::parse(al, s);
    return LFortran::pickle(*result);
}


TEST_CASE("Check pickle()") {
    Allocator al(4*1024);

    CHECK(p(al, "2*x")   == "(BinOp Mul 2 x)");
    CHECK(p(al, "(2*x)") == "(BinOp Mul 2 x)");
    CHECK(p(al, "3*x**y") == "(BinOp Mul 3 (BinOp Pow x y))");
    CHECK(p(al, "x = 2*y") == "(Assignment x (BinOp Mul 2 y))");
}
