#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>

bool p(Allocator &al, const std::string &s)
{
    LFortran::AST::ast_t* result;
    result = LFortran::parse2(al, s);
    LFortran::ast_to_asr(*result);
    return true;
}

#define P(x) p(al, x)

TEST_CASE("Functions") {
    Allocator al(4*1024);

    CHECK(P(R"(function f()
integer :: f
f = 5
end function)"));
}
