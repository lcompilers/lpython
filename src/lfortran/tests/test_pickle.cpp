#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/parser/parser.h>

void section(const std::string &s)
{
    std::cout << color(LFortran::style::bold) << color(LFortran::fg::blue) << s << color(LFortran::style::reset) << color(LFortran::fg::reset) << std::endl;
}

namespace {
    class ParserError0 {
    };
}

std::string p(Allocator &al, const std::string &s)
{
    LFortran::AST::TranslationUnit_t* result;
    LFortran::diag::Diagnostics diagnostics;
    LFortran::Result<LFortran::AST::TranslationUnit_t*> res
        = LFortran::parse(al, s, diagnostics);
    if (res.ok) {
        result = res.result;
    } else {
        throw ParserError0();
    }
    LFortran::AST::ast_t* ast;
    if (result->n_items >= 1) {
        ast = result->m_items[0];
    } else {
        ast = (LFortran::AST::ast_t*)result;
    }
    std::string pickle = LFortran::pickle(*ast);
    std::string src = LFortran::ast_to_src(*result);

    /*
    // Print the test nicely:
    section("--------------------------------------------------------------------------------");
    section("SRC:");
    std::cout << s << std::endl;
    section("SRC -> AST:");
    std::cout << pickle << std::endl;
    section("AST -> SRC:");
    std::cout << src << std::endl;
    */

    return pickle;
}

#define P(x) p(al, x)

TEST_CASE("Names") {
    Allocator al(4*1024);

    CHECK(P("2*y")   == "(* 2 y)");
    CHECK(P("2*yz")   == "(* 2 yz)");
    CHECK(P("abc*xyz")   == "(* abc xyz)");
    CHECK(P("abc*function")   == "(* abc function)");
    CHECK(P("abc*subroutine")   == "(* abc subroutine)");

    CHECK_THROWS_AS(P("abc*2xyz"), ParserError0);
}

TEST_CASE("Symbolic expressions") {
    Allocator al(4*1024);

    CHECK(P("2*x")   == "(* 2 x)");
    CHECK(P("(2*x)") == "(* 2 x)");
    CHECK(P("3*x**y") == "(* 3 (** x y))");
    CHECK(P("a+b*c") == "(+ a (* b c))");
    CHECK(P("(a+b)*c") == "(* (+ a b) c)");

    CHECK_THROWS_AS(P("2*"), ParserError0);
    CHECK_THROWS_AS(P("(2*x"), ParserError0);
    CHECK_THROWS_AS(P("2*x)"), ParserError0);
    CHECK_THROWS_AS(P("3*x**"), ParserError0);
}

TEST_CASE("Symbolic assignments") {
    Allocator al(4*1024);
    CHECK(P("x = y") == "(= 0 x y ())");
    CHECK(P("x = 2*y") == "(= 0 x (* 2 y) ())");

    CHECK_THROWS_AS(P("x ="), ParserError0);
    CHECK_THROWS_AS(P("x = 2*"), ParserError0);
    CHECK_THROWS_AS(P(" = 2*y"), ParserError0);
}

TEST_CASE("Arithmetics") {
    Allocator al(4*1024);

    CHECK_THROWS_AS(P("1+2**(*4)"), ParserError0);
    CHECK_THROWS_AS(P("1x"), ParserError0);
    CHECK_THROWS_AS(P("1+"), ParserError0);
    CHECK_THROWS_AS(P("(1+"), ParserError0);
    CHECK_THROWS_AS(P("(1+2"), ParserError0);
    CHECK_THROWS_AS(P("1+2*"), ParserError0);
    CHECK_THROWS_AS(P("f(3+6"), ParserError0);
}

TEST_CASE("Comparison") {
    Allocator al(4*1024);

    CHECK(P("1 == 2") == "(== 1 2)");
    CHECK(P("1 /= 2") == "(/= 1 2)");
    CHECK(P("1 < 2") == "(< 1 2)");
    CHECK(P("1 <= 2") == "(<= 1 2)");
    CHECK(P("1 > 2") == "(> 1 2)");
    CHECK(P("1 >= 2") == "(>= 1 2)");

    CHECK(P("1 .eq. 2") == "(== 1 2)");
    CHECK(P("1 .ne. 2") == "(/= 1 2)");
    CHECK(P("1 .lt. 2") == "(< 1 2)");
    CHECK(P("1 .le. 2") == "(<= 1 2)");
    CHECK(P("1 .gt. 2") == "(> 1 2)");
    CHECK(P("1 .ge. 2") == "(>= 1 2)");

    CHECK(P("1.eq.2") == "(== 1 2)");
    CHECK(P("1.ne.2") == "(/= 1 2)");
    CHECK(P("1.lt.2") == "(< 1 2)");
    CHECK(P("1.le.2") == "(<= 1 2)");
    CHECK(P("1.gt.2") == "(> 1 2)");
    CHECK(P("1.ge.2") == "(>= 1 2)");

    CHECK(P("1 == 2 + 3") == "(== 1 (+ 2 3))");
    CHECK(P("1 + 3*4 <= 2 + 3") == "(<= (+ 1 (* 3 4)) (+ 2 3))");
    CHECK(P("1 .eq. 2 + 3") == "(== 1 (+ 2 3))");
    CHECK(P("1 + 3*4 .le. 2 + 3") == "(<= (+ 1 (* 3 4)) (+ 2 3))");
    CHECK(P("1.eq.2 + 3") == "(== 1 (+ 2 3))");
    CHECK(P("1 + 3*4.le.2 + 3") == "(<= (+ 1 (* 3 4)) (+ 2 3))");

    // These are not valid Fortran, but we test that the parser follows the
    // precedence rules correctly
    CHECK(P("1 == 2 + 3 == 2") == "(== (== 1 (+ 2 3)) 2)");
    CHECK(P("(1 == 2) + 3") == "(+ (== 1 2) 3)");
}


TEST_CASE("Multiple units") {
    Allocator al(4*1024);
    LFortran::diag::Diagnostics diagnostics;
    LFortran::AST::TranslationUnit_t* results;
    std::string s = R"(x = x+1
        y = z+1)";
    results = LFortran::TRY(LFortran::parse(al, s, diagnostics));
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= 0 x (+ x 1) ())");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= 0 y (+ z 1) ())");

    s = "x = x+1; ; y = z+1";
    results = LFortran::TRY(LFortran::parse(al, s, diagnostics));
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= 0 x (+ x 1) (TriviaNode [] [(Semicolon) (Semicolon)]))");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= 0 y (+ z 1) ())");

    s = R"(x = x+1;

    ; y = z+1)";
    results = LFortran::TRY(LFortran::parse(al, s, diagnostics));
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= 0 x (+ x 1) (TriviaNode [] [(Semicolon) (EndOfLine) (EndOfLine) (Semicolon)]))");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= 0 y (+ z 1) ())");

    s = R"(x+1
    y = z+1
    a)";
    results = LFortran::TRY(LFortran::parse(al, s, diagnostics));
    CHECK(results->n_items == 3);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(+ x 1)");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= 0 y (+ z 1) ())");
    CHECK(LFortran::pickle(*results->m_items[2]) == "a");

    s = R"(function g()
    x = y
    x = 2*y
    end function
    s = x
    y = z+1
    a)";
    results = LFortran::TRY(LFortran::parse(al, s, diagnostics));
    CHECK(results->n_items == 4);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(Function g [] [] () () () [] [] [] [] [(= 0 x y ()) (= 0 x (* 2 y) ())] [])");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= 0 s x ())");
    CHECK(LFortran::pickle(*results->m_items[2]) == "(= 0 y (+ z 1) ())");
    CHECK(LFortran::pickle(*results->m_items[3]) == "a");
}

TEST_CASE("if") {
    Allocator al(16*1024);

    CHECK_THROWS_AS(P(R"(subroutine g
    if (x) then
        end if = 5
    end if
    end subroutine)"), ParserError0);

    CHECK_THROWS_AS(P(R"(subroutine g
    if (else) then
        else = 5
    else if (else) then
        else if (else)
    else if (else) then
        else = 3
    end if
    end subroutine)"), ParserError0);

}


TEST_CASE("while") {
    Allocator al(4*1024);

    CHECK_THROWS_AS(P(
 R"(do while (x)
        end do = 5
    enddo)"), ParserError0);

}

TEST_CASE("do loop") {
    Allocator al(4*1024);


    CHECK(P(
 R"(do
    a = a + i
    b = 3)") == "do");
}

TEST_CASE("exit") {
    Allocator al(4*1024);

    // (exit) ... statement exit
    // exit   ... variable "exit"

    CHECK(P(
 R"(exit
    enddo)") == "exit");

}

TEST_CASE("cycle") {
    Allocator al(4*1024);

    CHECK(P(
 R"(cycle
    enddo)") == "cycle");

}

TEST_CASE("return") {
    Allocator al(4*1024);

    CHECK(P(
 R"(return
    enddo)") == "return");

}

TEST_CASE("declaration") {
    Allocator al(1024*1024);

    CHECK_THROWS_AS(P("integer, parameter, pointer x"),
            ParserError0);
    CHECK_THROWS_AS(P("integer x(5,:,:3,3:) = x y"), ParserError0);

}
