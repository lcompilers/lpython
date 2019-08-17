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

#define P(x) p(al, x)


TEST_CASE("Check pickle()") {
    Allocator al(4*1024);

    CHECK(P("2*x")   == "(BinOp Mul 2 x)");
    CHECK(P("(2*x)") == "(BinOp Mul 2 x)");
    CHECK(P("3*x**y") == "(BinOp Mul 3 (BinOp Pow x y))");
    CHECK(P("x = 2*y") == "(Assignment x (BinOp Mul 2 y))");
}

TEST_CASE("Arithmetics") {
    Allocator al(4*1024);

    CHECK(P("1+2*3")   == "(BinOp Add 1 (BinOp Mul 2 3))");
    CHECK(P("1+(2*3)") == "(BinOp Add 1 (BinOp Mul 2 3))");
    CHECK(P("1*2+3")   == "(BinOp Add (BinOp Mul 1 2) 3)");
    CHECK(P("(1+2)*3") == "(BinOp Mul (BinOp Add 1 2) 3)");
    CHECK(P("1+2*3**4")   == "(BinOp Add 1 (BinOp Mul 2 (BinOp Pow 3 4)))");
    CHECK(P("1+2*(3**4)") == "(BinOp Add 1 (BinOp Mul 2 (BinOp Pow 3 4)))");
    CHECK(P("1+(2*3)**4") == "(BinOp Add 1 (BinOp Pow (BinOp Mul 2 3) 4))");
    CHECK(P("(1+2*3)**4") == "(BinOp Pow (BinOp Add 1 (BinOp Mul 2 3)) 4)");
    CHECK(P("1+2**3*4")   == "(BinOp Add 1 (BinOp Mul (BinOp Pow 2 3) 4))");
    CHECK(P("1+2**(3*4)") == "(BinOp Add 1 (BinOp Pow 2 (BinOp Mul 3 4)))");
}
