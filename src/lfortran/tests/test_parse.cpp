#include <tests/doctest.h>

#include <iostream>
#include <chrono>

#include <lfortran/parser/parser.h>

using LFortran::parse;
using LFortran::AST::ast_t;
using LFortran::AST::expr_t;
using LFortran::AST::Name_t;
using LFortran::AST::BaseWalkVisitor;

class CountVisitor : public BaseWalkVisitor<CountVisitor>
{
    int c_;
public:
    CountVisitor() : c_{0} {}
    void visit_Name(const Name_t &x) { c_ += 1; }
    int get_count() {
        return c_;
    }
};

int count(const ast_t &b) {
    CountVisitor v;
    v.visit_ast(b);
    return v.get_count();
}


TEST_CASE("Test longer parser (N = 500)") {
    int N;
    N = 500;
    std::string text;
    std::string t0 = "(a*z+3+2*x + 3*y - x/(z**2-4) - x**(y**z))";
    text.reserve(22542);
    text = t0;
    std::cout << "Construct" << std::endl;
    for (int i = 0; i < N; i++) {
        text.append(" * " + t0);
    }
    Allocator al(1024*1024);
    std::cout << "Parse" << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    auto result = parse(al, text);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                     .count()
              << "ms" << std::endl;
    int c = count(*result);
    std::cout << "Count: " << c << std::endl;
    std::cout << "String size (bytes):      " << text.size() << std::endl;
    std::cout << "Allocator usage (bytes): " << al.size_current() << std::endl;
    CHECK(c == 4509);
}
