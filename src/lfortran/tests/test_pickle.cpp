#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>


TEST_CASE("Check pickle()") {
    Allocator al(4*1024);
    std::string s;
    LFortran::AST::ast_t* result;

    result = LFortran::parse(al, "2*x");
    s = LFortran::pickle(*result);
    CHECK(s == "(BinOp Mul 2 x)");


    result = LFortran::parse(al, "(2*x)");
    s = LFortran::pickle(*result);
    CHECK(s == "(BinOp Mul 2 x)");


    result = LFortran::parse(al, "3*x**y");
    s = LFortran::pickle(*result);
    CHECK(s == "(BinOp Mul 3 (BinOp Pow x y))");

    result = LFortran::parse(al, "x = 2*y");
    s = LFortran::pickle(*result);
    CHECK(s == "(Assignment x (BinOp Mul 2 y))");
}
