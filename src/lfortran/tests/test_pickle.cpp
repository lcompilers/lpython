#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>


TEST_CASE("Check pickle()") {
    Allocator al(4*1024);
    std::string s, r;
    LFortran::AST::ast_t* result;

    result = LFortran::parse(al, "2*x");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    r = "(BinOp Mul 2 x)";
    CHECK(s == r);


    result = LFortran::parse(al, "(2*x)");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    CHECK(s == r);


    result = LFortran::parse(al, "3*x**y");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    r = "(BinOp Mul 3 (BinOp Pow x y))";
    CHECK(s == r);

    result = LFortran::parse(al, "x = 2*y");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    r = "(Assignment x (BinOp Mul 2 y))";
    CHECK(s == r);
}
