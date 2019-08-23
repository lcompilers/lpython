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

    CHECK(P("2*y")   == "(* 2 y)");
    CHECK(P("2*yz")   == "(* 2 yz)");
    CHECK(P("abc*xyz")   == "(* abc xyz)");
    CHECK(P("abc*function")   == "(* abc function)");
    CHECK(P("abc*subroutine")   == "(* abc subroutine)");

    CHECK_THROWS_AS(P("abc*2xyz"), LFortran::ParserError);
}

TEST_CASE("Symbolic expressions") {
    Allocator al(4*1024);

    CHECK(P("2*x")   == "(* 2 x)");
    CHECK(P("(2*x)") == "(* 2 x)");
    CHECK(P("3*x**y") == "(* 3 (** x y))");
    CHECK(P("a+b*c") == "(+ a (* b c))");
    CHECK(P("(a+b)*c") == "(* (+ a b) c)");

    CHECK_THROWS_AS(P("2*"), LFortran::ParserError);
    CHECK_THROWS_AS(P("(2*x"), LFortran::ParserError);
    CHECK_THROWS_AS(P("2*x)"), LFortran::ParserError);
    CHECK_THROWS_AS(P("3*x**"), LFortran::ParserError);
}

TEST_CASE("Symbolic assignments") {
    Allocator al(4*1024);
    CHECK(P("x = y") == "(= x y)");
    CHECK(P("x = 2*y") == "(= x (* 2 y))");

    CHECK_THROWS_AS(P("x ="), LFortran::ParserError);
    CHECK_THROWS_AS(P("x = 2*"), LFortran::ParserError);
    CHECK_THROWS_AS(P(" = 2*y"), LFortran::ParserError);
}

TEST_CASE("Arithmetics") {
    Allocator al(4*1024);

    CHECK(P("1+2*3")   == "(+ 1 (* 2 3))");
    CHECK(P("1+(2*3)") == "(+ 1 (* 2 3))");
    CHECK(P("1*2+3")   == "(+ (* 1 2) 3)");
    CHECK(P("(1+2)*3") == "(* (+ 1 2) 3)");
    CHECK(P("1+2*3**4")   == "(+ 1 (* 2 (** 3 4)))");
    CHECK(P("1+2*(3**4)") == "(+ 1 (* 2 (** 3 4)))");
    CHECK(P("1+(2*3)**4") == "(+ 1 (** (* 2 3) 4))");
    CHECK(P("(1+2*3)**4") == "(** (+ 1 (* 2 3)) 4)");
    CHECK(P("1+2**3*4")   == "(+ 1 (* (** 2 3) 4))");
    CHECK(P("1+2**(3*4)") == "(+ 1 (** 2 (* 3 4)))");

    CHECK_THROWS_AS(P("1+2**(*4)"), LFortran::ParserError);
}

TEST_CASE("Comparison") {
    Allocator al(4*1024);

    CHECK(P("1 == 2") == "(== 1 2)");
    CHECK(P("1 /= 2") == "(/= 1 2)");
    CHECK(P("1 < 2") == "(< 1 2)");
    CHECK(P("1 <= 2") == "(<= 1 2)");
    CHECK(P("1 > 2") == "(> 1 2)");
    CHECK(P("1 >= 2") == "(>= 1 2)");

    CHECK(P("1 == 2 + 3") == "(== 1 (+ 2 3))");
    CHECK(P("1 + 3*4 <= 2 + 3") == "(<= (+ 1 (* 3 4)) (+ 2 3))");

    // These are not valid Fortran, but we test that the parser follows the
    // precedence rules correctly
    CHECK(P("1 == 2 + 3 == 2") == "(== (== 1 (+ 2 3)) 2)");
    CHECK(P("(1 == 2) + 3") == "(+ (== 1 2) 3)");
}

TEST_CASE("Subroutines") {
    Allocator al(4*1024);

    CHECK(P(R"(subroutine g
    x = y
    x = 2*y
    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine g
    x = y

    x = 2*y
    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine g

    x = y


    x = 2*y



    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine g
    x = y
    ;;;;;; ; ; ;
    x = 2*y
    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine g
    x = y;
    x = 2*y;
    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine g
    x = y; ;
    x = 2*y;; ;
    end subroutine)")   == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P("subroutine g; x = y; x = 2*y; end subroutine") == "(sub [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(subroutine f
    subroutine = y
    x = 2*subroutine
    end subroutine)")   == "(sub [] [(= subroutine y) (= x (* 2 subroutine))])");
}

TEST_CASE("Functions") {
    Allocator al(4*1024);

    CHECK(P(R"(function g
    x = y
    x = 2*y
    end function)")   == "(fn [] [(= x y) (= x (* 2 y))])");


    CHECK(P(R"(function g
    x = y; ;


    x = 2*y;; ;

    end function)")   == "(fn [] [(= x y) (= x (* 2 y))])");

    CHECK(P("function g; x = y; x = 2*y; end function") == "(fn [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(function f
    subroutine = y
    x = 2*subroutine
    end function)")   == "(fn [] [(= subroutine y) (= x (* 2 subroutine))])");
}

TEST_CASE("Programs") {
    Allocator al(4*1024);

    CHECK(P(R"(program g
    x = y
    x = 2*y
    end program)")   == "(prog g [] [(= x y) (= x (* 2 y))])");


    CHECK(P(R"(program g
    x = y; ;


    x = 2*y;; ;

    end program)")   == "(prog g [] [(= x y) (= x (* 2 y))])");

    CHECK(P("program g; x = y; x = 2*y; end program") == "(prog g [] [(= x y) (= x (* 2 y))])");

    CHECK(P(R"(program f
    subroutine = y
    x = 2*subroutine
    end program)")   == "(prog f [] [(= subroutine y) (= x (* 2 subroutine))])");

    CHECK(P(R"(program g
    x = y
    end program g)")   == "(prog g [] [(= x y)])");

    /*
    CHECK(P(
   R"(PROGRAM TESTFortran90
      integer stop ; stop = 1 ; do while ( stop .eq. 0 ) ; end do
      END PROGRAM TESTFortran90)") == "");
    */
}

TEST_CASE("Multiple units") {
    Allocator al(4*1024);
    std::vector<LFortran::AST::ast_t*> results;
    std::string s = R"(x = x+1
        y = z+1)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(= y (+ z 1))");

    s = "x = x+1; ; y = z+1";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(= y (+ z 1))");

    s = R"(x = x+1;

    ; y = z+1)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 2);
    CHECK(LFortran::pickle(*results[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results[1]) == "(= y (+ z 1))");

    s = R"(x+1
    y = z+1
    a)";
    results = LFortran::parsen(al, s);
    CHECK(results.size() == 3);
    CHECK(LFortran::pickle(*results[0]) == "(+ x 1)");
    CHECK(LFortran::pickle(*results[1]) == "(= y (+ z 1))");
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
    CHECK(LFortran::pickle(*results[0]) == "(fn [] [(= x y) (= x (* 2 y))])");
    CHECK(LFortran::pickle(*results[1]) == "(= s x)");
    CHECK(LFortran::pickle(*results[2]) == "(= y (+ z 1))");
    CHECK(LFortran::pickle(*results[3]) == "a");
}

TEST_CASE("if") {
    Allocator al(16*1024);

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
        b = 4
    end if
    end subroutine)")   == "(sub [] [(if x [(= a 5) (= b 4)] [])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    end   If
    end subroutine)")   == "(sub [] [(if x [(= a 5)] [])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    ENDIF
    end subroutine)")   == "(sub [] [(if x [(= a 5)] [])])");

    CHECK(P(R"(subroutine g
    if (x) then
        endif = 5
    ENDIF
    end subroutine)")   == "(sub [] [(if x [(= endif 5)] [])])");

    CHECK_THROWS_AS(P(R"(subroutine g
    if (x) then
        end if = 5
    end if
    end subroutine)"), LFortran::ParserError);

    CHECK(P(R"(subroutine g
    if (else) then
        a = 5
        b = 4
    end if
    end subroutine)")   == "(sub [] [(if else [(= a 5) (= b 4)] [])])");

    CHECK(P(R"(subroutine g
    if (else) then
        then = 5
        else = 4
    end if
    end subroutine)")   == "(sub [] [(if else [(= then 5) (= else 4)] [])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else
        b = 4
    end if
    end subroutine)")   == "(sub [] [(if x [(= a 5)] [(= b 4)])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
        c = 7
    else
        b = 4
        e = 5
    end if
    end subroutine)")   == "(sub [] [(if x [(= a 5) (= c 7)] [(= b 4) (= e 5)])])");

    CHECK(P(R"(subroutine g
    if (else) then
        else = 5
    else
        else = 4
    end if
    end subroutine)")   == "(sub [] [(if else [(= else 5)] [(= else 4)])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    end if
    end subroutine)") == "(sub [] [(if x [(= a 5)] [(if y [(= b 4)] [])])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    else if (z) then
        c = 3
    end if
    end subroutine)") == "(sub [] [(if x [(= a 5)] [(if y [(= b 4)] [(if z [(= c 3)] [])])])])");

    CHECK_THROWS_AS(P(R"(subroutine g
    if (else) then
        else = 5
    else if (else) then
        else if (else) then
    else if (else) then
        else = 3
    end if
    end subroutine)"), LFortran::ParserError);

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    else
        c = 3
    end if
    end subroutine)") == "(sub [] [(if x [(= a 5)] [(if y [(= b 4)] [(= c 3)])])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
        if (y) then
            b = 4
        else
            c = 3
        end if
    end if
    end subroutine)") == "(sub [] [(if x [(= a 5) (if y [(= b 4)] [(= c 3)])] [])])");

    CHECK(P(R"(subroutine g
    if (x) then
        a = 5
        if (y) then
            b = 4
        end if
    else
        c = 3
    end if
    end subroutine)") == "(sub [] [(if x [(= a 5) (if y [(= b 4)] [])] [(= c 3)])])");

    CHECK(P(
 R"(if (x) then
        a = 5
        if (y) then
            b = 4
        end if
    else
        c = 3
    end if)") == "(if x [(= a 5) (if y [(= b 4)] [])] [(= c 3)])");
}


TEST_CASE("while") {
    Allocator al(4*1024);

    CHECK(P(
 R"(do while (x)
        a = 5
    end do)") == "(while x [(= a 5)])");

    CHECK(P(
 R"(do while (x)
        a = 5
    enddo)") == "(while x [(= a 5)])");

    CHECK(P(
 R"(do while (x)
        do = 5
    enddo)") == "(while x [(= do 5)])");

    CHECK(P(
 R"(do while (x)
        end = 5
    enddo)") == "(while x [(= end 5)])");

    CHECK(P(
 R"(do while (x)
        enddo = 5
    enddo)") == "(while x [(= enddo 5)])");

    CHECK_THROWS_AS(P(
 R"(do while (x)
        end do = 5
    enddo)"), LFortran::ParserError);
}

TEST_CASE("do loop") {
    Allocator al(4*1024);

    CHECK(P(
 R"(do i = 1, 5
        a = a + i
    end do)") == "(do i 1 5 () [(= a (+ a i))])");

    CHECK(P(
 R"(do i = 1, 5, 2
        a = a + i
    end do)") == "(do i 1 5 2 [(= a (+ a i))])");

    CHECK(P(
 R"(do
        a = a + i
        b = 3
    end do)") == "(do () () () () [(= a (+ a i)) (= b 3)])");

    CHECK(P(R"(subroutine g
    do
        a = a + i
        b = 3
    enddo
    end subroutine)") == "(sub [] [(do () () () () [(= a (+ a i)) (= b 3)])])");

    CHECK(P(
 R"(do
        a = a + i
        b = 3
    enddo)") == "(do () () () () [(= a (+ a i)) (= b 3)])");

    CHECK(P(
 R"(do
        do = a + i
        enddo = 3
    enddo)") == "(do () () () () [(= do (+ a i)) (= enddo 3)])");

    CHECK(P("do; a = a + i; b = 3; end do") == "(do () () () () [(= a (+ a i)) (= b 3)])");

    CHECK(P("do") == "do");

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
 R"(do i = 1, 5
        exit
    end do)") == "(do i 1 5 () [(exit)])");

    CHECK(P(
 R"(do
        exit
    enddo)") == "(do () () () () [(exit)])");

    CHECK(P(
 R"(do while (x)
        exit
    end do)") == "(while x [(exit)])");

    CHECK(P(
 R"(exit
    enddo)") == "exit");

    CHECK(P(
 R"(do i = 1, 5
        exit = 5
    end do)") == "(do i 1 5 () [(= exit 5)])");

    CHECK(P("exit") == "exit");
    CHECK(P("exit+1") == "(+ exit 1)");
    CHECK(P("exit=1") == "(= exit 1)");
    CHECK(P("a=exit") == "(= a exit)");
}

TEST_CASE("cycle") {
    Allocator al(4*1024);

    CHECK(P(
 R"(do i = 1, 5
        cycle
    end do)") == "(do i 1 5 () [(cycle)])");

    CHECK(P(
 R"(do while (x)
        cycle
    end do)") == "(while x [(cycle)])");

    CHECK(P(
 R"(cycle
    enddo)") == "cycle");

    CHECK(P(
 R"(do i = 1, 5
        cycle = 5
    end do)") == "(do i 1 5 () [(= cycle 5)])");

    CHECK(P("cycle") == "cycle");
    CHECK(P("cycle+1") == "(+ cycle 1)");
    CHECK(P("cycle=1") == "(= cycle 1)");
    CHECK(P("a=cycle") == "(= a cycle)");
}

TEST_CASE("return") {
    Allocator al(4*1024);

    CHECK(P(R"(subroutine g
    x = y
    return
    x = 2*y
    end subroutine)")   == "(sub [] [(= x y) (return) (= x (* 2 y))])");

    CHECK(P(
 R"(do i = 1, 5
        return
    end do)") == "(do i 1 5 () [(return)])");

    CHECK(P(
 R"(do while (x)
        return
    end do)") == "(while x [(return)])");

    CHECK(P(
 R"(return
    enddo)") == "return");

    CHECK(P(
 R"(do i = 1, 5
        return = 5
    end do)") == "(do i 1 5 () [(= return 5)])");

    CHECK(P("return") == "return");
    CHECK(P("return+1") == "(+ return 1)");
    CHECK(P("return=1") == "(= return 1)");
    CHECK(P("a=return") == "(= a return)");
}

TEST_CASE("declaration") {
    Allocator al(4*1024);

    CHECK(P("integer x") == "(decl x integer)");
    CHECK(P("character x") == "(decl x character)");
    CHECK(P("real x") == "(decl x real)");
    CHECK(P("complex x") == "(decl x complex)");
    CHECK(P("logical x") == "(decl x logical)");
    CHECK(P("type x") == "(decl x type)");

    CHECK(P(R"(function g
    integer x
    x = 1
    end function)") == "(fn [(decl x integer)] [(= x 1)])");

    CHECK(P(R"(function g
    integer x
    real x
    x = 1
    end function)") == "(fn [(decl x integer) (decl x real)] [(= x 1)])");

    CHECK(P(R"(subroutine g
    integer x
    x = 1
    end subroutine)") == "(sub [(decl x integer)] [(= x 1)])");

    CHECK(P(R"(subroutine g
    integer x
    character x
    x = 1
    end subroutine)") == "(sub [(decl x integer) (decl x character)] [(= x 1)])");

    CHECK(P(R"(program g
    integer x
    x = 1
    end program)") == "(prog g [(decl x integer)] [(= x 1)])");

    CHECK(P(R"(program g
    integer x
    complex x
    x = 1
    end program)") == "(prog g [(decl x integer) (decl x complex)] [(= x 1)])");
}
