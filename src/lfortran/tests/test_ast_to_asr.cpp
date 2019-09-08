#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>

std::string p(Allocator &al, const std::string &s)
{
    LFortran::AST::ast_t* ast = LFortran::parse2(al, s);
    LFortran::ASR::asr_t* asr = LFortran::ast_to_asr(al, *ast);
    return LFortran::pickle(*asr);
}

#define P(x) p(al, x)

TEST_CASE("Functions") {
    Allocator al(4*1024);

    CHECK(P(R"(function f()
integer :: f
f = 5
end function)") == "(fn f [] [] () (variable f () Unimplemented (integer Unimplemented [])) () Unimplemented)");
}
