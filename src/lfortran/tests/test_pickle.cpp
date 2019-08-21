#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>

std::string p(Allocator &al, const std::string &s)
{
    LFortran::AST::ast_t* result;
    result = LFortran::parse2(al, s);
    return LFortran::pickle(*result);
}

#define P(x) p(al, x)

TEST_CASE("Names") {
    Allocator al(4*1024);

    CHECK(P("2*y")   == "(BinOp Mul 2 y)");
    CHECK(P("2*yz")   == "(BinOp Mul 2 yz)");
    CHECK(P("abc*xyz")   == "(BinOp Mul abc xyz)");
    CHECK(P("abc*function")   == "(BinOp Mul abc function)");
    CHECK(P("abc*subroutine")   == "(BinOp Mul abc subroutine)");

    CHECK_THROWS_AS(P("abc*2xyz"), LFortran::ParserError);
}

TEST_CASE("Symbolic expressions") {
    Allocator al(4*1024);

    CHECK(P("2*x")   == "(BinOp Mul 2 x)");
    CHECK(P("(2*x)") == "(BinOp Mul 2 x)");
    CHECK(P("3*x**y") == "(BinOp Mul 3 (BinOp Pow x y))");
    CHECK(P("a+b*c") == "(BinOp Add a (BinOp Mul b c))");
    CHECK(P("(a+b)*c") == "(BinOp Mul (BinOp Add a b) c)");

    CHECK_THROWS_AS(P("2*"), LFortran::ParserError);
    CHECK_THROWS_AS(P("(2*x"), LFortran::ParserError);
    CHECK_THROWS_AS(P("2*x)"), LFortran::ParserError);
    CHECK_THROWS_AS(P("3*x**"), LFortran::ParserError);
}

TEST_CASE("Symbolic assignments") {
    Allocator al(4*1024);
    CHECK(P("x = y") == "(Assignment x y)");
    CHECK(P("x = 2*y") == "(Assignment x (BinOp Mul 2 y))");

    CHECK_THROWS_AS(P("x ="), LFortran::ParserError);
    CHECK_THROWS_AS(P("x = 2*"), LFortran::ParserError);
    CHECK_THROWS_AS(P(" = 2*y"), LFortran::ParserError);
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

    CHECK_THROWS_AS(P("1+2**(*4)"), LFortran::ParserError);
}

TEST_CASE("Subroutines") {
    Allocator al(4*1024);

    CHECK(P(R"(subroutine g
    x = y
    x = 2*y
    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine g
    x = y

    x = 2*y
    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine g

    x = y


    x = 2*y



    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine g
    x = y
    ;;;;;; ; ; ;
    x = 2*y
    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine g
    x = y;
    x = 2*y;
    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine g
    x = y; ;
    x = 2*y;; ;
    end subroutine)")   == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P("subroutine g; x = y; x = 2*y; end subroutine") == "(Subroutine 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(subroutine f
    subroutine = y
    x = 2*subroutine
    end subroutine)")   == "(Subroutine 2 (Assignment subroutine y)(Assignment x (BinOp Mul 2 subroutine)))");
}

TEST_CASE("Functions") {
    Allocator al(4*1024);

    CHECK(P(R"(function g
    x = y
    x = 2*y
    end function)")   == "(Function 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");


    CHECK(P(R"(function g
    x = y; ;


    x = 2*y;; ;

    end function)")   == "(Function 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P("function g; x = y; x = 2*y; end function") == "(Function 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(function f
    subroutine = y
    x = 2*subroutine
    end function)")   == "(Function 2 (Assignment subroutine y)(Assignment x (BinOp Mul 2 subroutine)))");
}

TEST_CASE("Programs") {
    Allocator al(4*1024);

    CHECK(P(R"(program g
    x = y
    x = 2*y
    end program)")   == "(Program 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");


    CHECK(P(R"(program g
    x = y; ;


    x = 2*y;; ;

    end program)")   == "(Program 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P("program g; x = y; x = 2*y; end program") == "(Program 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");

    CHECK(P(R"(program f
    subroutine = y
    x = 2*subroutine
    end program)")   == "(Program 2 (Assignment subroutine y)(Assignment x (BinOp Mul 2 subroutine)))");
}

TEST_CASE("Multiple units") {
    Allocator al(4*1024);
    std::vector<LFortran::AST::ast_t*> results;
    std::string s = R"(x = x+1
        y = z+1)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(Assignment x (BinOp Add x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(Assignment y (BinOp Add z 1))");

    s = "x = x+1; ; y = z+1";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(Assignment x (BinOp Add x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(Assignment y (BinOp Add z 1))");

    s = R"(x = x+1;

    ; y = z+1)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(Assignment x (BinOp Add x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(Assignment y (BinOp Add z 1))");

    s = R"(x+1
    y = z+1
    a)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 3);
    CHECK(LFortran::pickle(*results[0]) == "(BinOp Add x 1)");
    CHECK(LFortran::pickle(*results[1]) == "(Assignment y (BinOp Add z 1))");
    CHECK(LFortran::pickle(*results[2]) == "a");

    s = R"(function g
    x = y
    x = 2*y
    end function
    s = x
    y = z+1
    a)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 4);
    CHECK(LFortran::pickle(*results[0]) == "(Function 2 (Assignment x y)(Assignment x (BinOp Mul 2 y)))");
    CHECK(LFortran::pickle(*results[1]) == "(Assignment s x)");
    CHECK(LFortran::pickle(*results[2]) == "(Assignment y (BinOp Add z 1))");
    CHECK(LFortran::pickle(*results[3]) == "a");
}

TEST_CASE("if") {
    Allocator al(4*1024);

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
        b = 4
    end if
    end subroutine)")   == "(Subroutine 1 (If x (Assignment a 5)(Assignment b 4)))");

    CHECK(P(R"(subroutine g
    if (else) then
        a = 5
        b = 4
    end if
    end subroutine)")   == "(Subroutine 1 (If else (Assignment a 5)(Assignment b 4)))");

    CHECK(P(R"(subroutine g
    if (else) then
        then = 5
        else = 4
    end if
    end subroutine)")   == "(Subroutine 1 (If else (Assignment then 5)(Assignment else 4)))");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else
        b = 4
    end if
    end subroutine)")   == "(Subroutine 1 (If x (Assignment a 5) (Assignment b 4)))");

    CHECK(P(R"(subroutine g
    if (else) then
        else = 5
    else
        else = 4
    end if
    end subroutine)")   == "(Subroutine 1 (If else (Assignment else 5) (Assignment else 4)))");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    end if
    end subroutine)") == "(Subroutine 1 (If x (Assignment a 5) (If y (Assignment b 4))))");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    else if (z) then
        c = 3
    end if
    end subroutine)") == "(Subroutine 1 (If x (Assignment a 5) (If y (Assignment b 4) (If z (Assignment c 3)))))");

    CHECK_THROWS_AS(P(R"(subroutine g
    if (else) then
        else = 5
    else if (else) then
        else if (else) then
    else if (else) then
        else = 3
    end if
    end subroutine)"), LFortran::ParserError);
}
