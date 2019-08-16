#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>


TEST_CASE("Check pickle()") {
    Allocator al(4*1024);
    std::string s, r;
    LFortran::AST::expr_t* result;

    result = LFortran::parse(al, "2*x");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    r = "1Mul50x";
    CHECK(s == r);


    result = LFortran::parse(al, "(2*x)");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    CHECK(s == r);


    result = LFortran::parse(al, "2*x**y");
    s = LFortran::pickle(*result);
    std::cout << s << std::endl;
    r = "1Mul501Powxy";
    CHECK(s == r);
}
