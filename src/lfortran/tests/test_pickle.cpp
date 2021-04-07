#include <tests/doctest.h>
#include <iostream>

#include <lfortran/pickle.h>
#include <lfortran/ast_to_src.h>
#include <lfortran/parser/parser.h>

void section(const std::string &s)
{
    std::cout << color(LFortran::style::bold) << color(LFortran::fg::blue) << s << color(LFortran::style::reset) << color(LFortran::fg::reset) << std::endl;
}

std::string p(Allocator &al, const std::string &s)
{
    LFortran::AST::TranslationUnit_t* result;
    result = LFortran::parse2(al, s);
    LFortran::AST::ast_t* ast;
    if (result->n_items >= 1) {
        ast = result->m_items[0];
    } else {
        ast = (LFortran::AST::ast_t*)result;
    }
    std::string pickle = LFortran::pickle(*ast);
    std::string src = LFortran::ast_to_src(*result);

    // Print the test nicely:
    section("--------------------------------------------------------------------------------");
    section("SRC:");
    std::cout << s << std::endl;
    section("SRC -> AST:");
    std::cout << pickle << std::endl;
    section("AST -> SRC:");
    std::cout << src << std::endl;

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

    CHECK_THROWS_AS(P("1+2**(*4)"), LFortran::ParserError);
    CHECK_THROWS_AS(P("1x"), LFortran::ParserError);
    CHECK_THROWS_AS(P("1+"), LFortran::ParserError);
    CHECK_THROWS_AS(P("(1+"), LFortran::ParserError);
    CHECK_THROWS_AS(P("(1+2"), LFortran::ParserError);
    CHECK_THROWS_AS(P("1+2*"), LFortran::ParserError);
    CHECK_THROWS_AS(P("f(3+6"), LFortran::ParserError);
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
    LFortran::AST::TranslationUnit_t* results;
    std::string s = R"(x = x+1
        y = z+1)";
    results = LFortran::parse(al, s);
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= y (+ z 1))");

    s = "x = x+1; ; y = z+1";
    results = LFortran::parse(al, s);
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= y (+ z 1))");

    s = R"(x = x+1;

    ; y = z+1)";
    results = LFortran::parse(al, s);
    CHECK(results->n_items == 2);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(= x (+ x 1))");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= y (+ z 1))");

    s = R"(x+1
    y = z+1
    a)";
    results = LFortran::parse(al, s);
    CHECK(results->n_items == 3);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(+ x 1)");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= y (+ z 1))");
    CHECK(LFortran::pickle(*results->m_items[2]) == "a");

    s = R"(function g()
    x = y
    x = 2*y
    end function
    s = x
    y = z+1
    a)";
    results = LFortran::parse(al, s);
    CHECK(results->n_items == 4);
    CHECK(LFortran::pickle(*results->m_items[0]) == "(Function g [] [] () () [] [] [(= x y) (= x (* 2 y))] [])");
    CHECK(LFortran::pickle(*results->m_items[1]) == "(= s x)");
    CHECK(LFortran::pickle(*results->m_items[2]) == "(= y (+ z 1))");
    CHECK(LFortran::pickle(*results->m_items[3]) == "a");
}

TEST_CASE("if") {
    Allocator al(16*1024);

    CHECK_THROWS_AS(P(R"(subroutine g
    if (x) then
        end if = 5
    end if
    end subroutine)"), LFortran::ParserError);

    CHECK_THROWS_AS(P(R"(subroutine g
    if (else) then
        else = 5
    else if (else) then
        else if (else)
    else if (else) then
        else = 3
    end if
    end subroutine)"), LFortran::ParserError);

}


TEST_CASE("while") {
    Allocator al(4*1024);

    CHECK_THROWS_AS(P(
 R"(do while (x)
        end do = 5
    enddo)"), LFortran::ParserError);

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
            LFortran::ParserError);
    CHECK_THROWS_AS(P("integer x(5,:,:3,3:) = x y"), LFortran::ParserError);

}

TEST_CASE("Lists of tests") {
    Allocator al(1024*1024);
    std::vector<std::string> v = {
        // Arithmetics
        // +, *, **
        "1+2*3",
        "1+(2*3)",
        "1*2+3",
        "(1+2)*3",
        "1+2*3**4",
        "1+2*(3**4)",
        "1+(2*3)**4",
        "(1+2*3)**4",
        "1+2**3*4",
        "1+2**(3*4)",

        // -
        "1-2",
        "1-2-3",
        "1-2*3",
        "1-2**3",

        // Unary -
        "-2+1",
        "-2*3+1",
        "-2**3+1",
        "1+(-2)",
        "1+(-2*3)",
        "1+(-2-3)",
        "3*(-2)",
        "(-2)*3",
        "-2*3",

        // Unary +
        "+2+1",
        "+2*3+1",
        "+2**3+1",
        "1+(+2)",
        "1+(+2*3)",
        "1+(+2-3)",
        "3*(+2)",
        "(+2)*3",
        "+2*3",

        // ---------------------------------------------------
        // Logical

        ".true.",
        ".false.",

        // ------------------------------------------------------------
        // Function calls or arrays:
        "f(a)",
        "f(1+1)",
        "f(a, b, 1)",
        "log(r2)",
        "u * sqrt(-2*log(r2)/r2)",

        // Only arrays
        "f(a,:,:,a)",
        "f(a:,1:,:2,1:a,:)",

        // Only functions
        "f()",
        "f(a, arg1=b+1, arg2=.true.)",

        // ----------------------------------------------------------
        "[1]",
        "[1,b]",
        "[1,b,c]",
        "[f(a),b+c,c**2+1]",

        // --------------------
        // Tests from Python parser
        "1",
        "1.",
        "1._dp",
        "1.03_dp",
        "1.e5_dp",
        "2+3",
        "(1+3)*4",
        "1+3*4",
        "x",
        "yx",
        "x+y",
        "2+x",
        "(x+y)**2",
        "(x+y)*3",
        "x+y*3",
        "(1+2+a)*3",
        "f(3)+6",
        "f(3+6)",
        "real(b, dp)",
        "2*u-1",
        "sum(u**2)",
        "u(2)",
        "u * sqrt(-2*log(r2)/r2)",
        ".not. first",
        "a - 1._dp/3",
        "1/sqrt(9*d)",
        "(1 + c*x)**3",
        "i + 1",
        R"("s")",
        R"("some test")",
        "a(3:5,i:j)",
        "b(:)",
        "a(:5,i:j) + b(1:)",
        "[1, 2, 3, i]",
        "f()",
        "x%a",
        "x%a()",
        "x%b(i, j)",
        "y%c(5, :)",
        "x%f%a",
        "x%g%b(i, j)",
        "y%h%c(5, :)",
        R"( "a'b'c" )",
        R"( 'a"b"c')",
        R"( 'a""bc""x')",
        R"( "a""c" )",
        R"( "a""b""c" )",
        R"( """zippo""" )",
        R"( 'a''c')",
        R"( 'a''b''c')",
        R"( '''zippo''')",
        R"( "aaa" // str(x) // "bb" )",
        R"( "a" // "b" )",

        "1 .and. 2",
        "a .and. b",
        "a == 1 .and. b == 2",
        "a == 1 .or. b == 2",
        "a .and. b .and. c",
        "a .or. b .or. c",
        ".not. (a == 1)",
        "(a == 1) .and. .not. (b == 2)",
        "a .eqv. b",
        "a .neqv. b",

        // ----------------------------------------------
        // Declarations

        "integer x",
        "integer :: x",
        "integer, parameter :: x",
        "integer, parameter, pointer :: x",
        "character x",
        "character(len=*) x",
        "character(len=*) :: c",
        "real x",
        "real :: a",
        "real(dp) x",
        "real(dp) :: x",
        "real(kind=dp) :: x",
        "real(dp), intent(in) :: x",
        "real(dp), intent(out) :: x",
        "real(dp), intent(inout) :: x",
        "real(dp) :: a, b",
        "real(dp) :: a(9,10), b(10)",
        "real(dp) :: a(m,n)",
        "real(dp), allocatable :: a(:,:)",
        "real(dp) y = 5",
        "real(c_double) :: f",
        "complex x",
        "logical x",
        "type(y) x",
        "type(xx), intent(inout) :: x, y",

        "integer x = 3",
        "integer x(5)",
        "integer x(5:)",
        "integer :: x(5:)",
        "integer :: a(9,10), b(10)",
        "integer x(:5)",
        "integer x(:)",
        "integer x(3:5)",
        "integer x(5,:,:3,3:)",
        "integer, dimension(5,:,:3,3:) :: x",
        "integer, dimension(9,10) :: c",
        "integer, dimension(:,:), intent(in) :: e",
        "integer x(5,:,:3,3:) = 3",
        "integer x(5,:,:3,3:) = 3+2",
        "integer x(5,:,:3,3:) = 3, y(:3)=4",
        "integer x(5,:,:3,3:) = x",
        "integer(c_int) :: i",

        R"(function g()
            integer x
            x = 1
            end function)",

        R"(subroutine g
            integer x, y
            end subroutine)",

        R"(subroutine g
            integer x, y, z
            end subroutine)",

        R"(function g()
            integer x
            real y, z
            x = 1
            end function)",

        R"(subroutine g
            integer x
            x = 1
            end subroutine)",

        R"(subroutine g
            integer x
            character x
            x = 1
            end subroutine)",

        R"(program g
            integer x
            x = 1
            end program)",

        R"(program g
            integer x
            complex x
            x = 1
            end program)",

        // ---------------------------------------------------
        // Statements

        "call random_number(u)",
        "call a%random_number(u)",
        "call a%b%random_number(u)",
        "u = 2*u-1",
        "r2 = sum(u**2)",
        "u = u * sqrt(-2*log(r2)/r2)",
        "x = u(1)",
        "x = u(2)",
        "first = .not. first",
        "d = a - 1._dp/3",
        "c = 1/sqrt(9*d)",
        "v = (1 + c*x)**3",
        "fn_val = d*v",
        "exit",
        "cycle",
        "call randn(x(i))",
        "call randn(x)",
        "call random_number(U)",
        "call rand_gamma0(a, .true., x)",
        "call rand_gamma0(a, .true., x(1))",
        "call rand_gamma0(a, .false., x(i))",
        "call rand_gamma_vector_n(a, size(x), x)",
        "call f(a=4, b=6, c=i)",
        "open(newunit=a, b, c)",
        "allocate(c(4), d(4))",
        "close(u)",
        "x = 1; y = 2",
        "y = 5; a = 1; x = u(2)",
        "a = 5",
        "x = 1; y = 2;",
        "y = 5; a = 1; x = u(2);",
        "a = 5;",
        "a;",
        "a;;",
        "a; ;",
        "a;b;",
        ";a",
        ";;a",
        "; ;a",
        "",
        " ",
        ";",
        ";;",
        "; ;",
        R"(stop "OK")",
        "stop 1",
        "stop",
        R"X(print "(i4)", 45, x)X",
        R"X(print "(i4)", 45)X",
        R"X(print "(i4)",)X",
        R"X(print "(i4)")X",
        R"(print *, 45, "sss", a+1)",
        "print *, a",
        "print *,",
        "print *",
        R"X(write (*,"(i4)") 45, x)X",
        R"X(write (*,"(i4)") 45)X",
        R"X(write (*,"(i4)"))X",
        R"(write (*,*) 45, "sss", a+1)",
        "write (*,*) a",
        "write (*,*)",
        R"X(write (u,"(i4)") 45, x)X",
        R"X(write (1,"(i4)") 45)X",
        R"X(write (13,"(i4)"))X",
        R"(write (u,*) 45, "sss", a+1)",
        "write (u,*) a",
        "write (u,*)",
        "write (u) a, b",
        "write (u) a",
        "write (u)",
        "x => y",
        "x => y(1:4, 5)",
        "call g(a(3:5,i:j), b(:))",
        "call g(a(:5,i:j), b(1:))",
        "c = [1, 2, 3, i]",
        "a = x%a",
        "b = x%b(i, j)",
        "c = y%c(5, :)",
        "a = x%f%a",
        "b = x%g%b(i, j)",
        "c = y%h%c(5, :)",
        "x%f%a = a",
        "x%g%b(i, j) = b",
        "y%h%c(5, :) = c",
        "call x%f%e()",

        // ------------------------------------------------------------
        // While loop
     R"(do while (x)
            a = 5
        end do)",
     R"(do while (x)
        end do)",
     R"(do while (x)
            a = 5
        enddo)",
     R"(do while (x)
            do = 5
        enddo)",
     R"(do while (x)
            end = 5
        enddo)",
     R"(do while (x)
            enddo = 5
        enddo)",
     R"(do while (x > 5)
            a = 5
        end do)",
        "do while (x > 5); a = 5; end do",
     R"(do while(x == y)
            i = i +1
            cycle
            exit
        end do)",

        // -----------------------------------------------------------
        // Select case
    R"(select case(k)
    case(1)
        call a()
    case(i)
        call b()
        call c()
end select
)",
    R"(select case(k)
    case(1)
        call a()
    case(i)
        call b()
        a = 5
    case default
        call c()
end select
)",

        // -----------------------------------------------------------
        // Where
        "where (a < 5) B = 1",
        R"(where (a < 5)
        B = 1
    end where
    )",
        R"(where (a < 5)
        B = 1
    else where
        B = 0
    end where
    )",
        R"(where (a < 5)
        B = 1
    else
        B = 0
    end where
    )",
        R"(where (a < 5)
        B = 1
    else where (a < 7)
        B = 0
    else where
        B = 3
    end where
    )",
        R"(where (a < 5)
        B = 1
    else where (a < 7)
        B = 0
    end where
    )",

        // -----------------------------------------------------------
        // Do loop
     R"(do i = 1, 5
            a = a + i
        end do)",
     R"(do i = 1, 5
        end do)",
     R"(do i = 1, 5, 2
            a = a + i
        end do)",
     R"(do
            a = a + i
            b = 3
        end do)",
     R"(subroutine g
        do
            a = a + i
            b = 3
        enddo
        end subroutine)",
     R"(do
            a = a + i
            b = 3
        enddo)",
     R"(do
            do = a + i
            enddo = 3
        enddo)",
        "do; a = a + i; b = 3; end do",
        "do",
        R"(subroutine a
    do
        x = 1
    end do
    end subroutine
    )",
        R"(subroutine a
    do i = 1, 5
        x = x + i
    end do
    end subroutine
    )",
        R"(subroutine a
    do i = 1, 5, -1
        x = i
    end do
    end subroutine
    )",

        // -------------------------------------------------------
        // Return
     R"(subroutine g
        x = y
        return
        x = 2*y
        end subroutine)",
     R"(do i = 1, 5
            return
        end do)",
     R"(do while (x)
            return
        end do)",
     R"(do i = 1, 5
            return = 5
        end do)",
        "return",
        "return+1",
        "return=1",
        "a=return",

        // -------------------------------------------------------
        // Cycle
     R"(do i = 1, 5
            cycle
        end do)",
     R"(do while (x)
            cycle
        end do)",
     R"(do i = 1, 5
            cycle = 5
        end do)",
        "cycle",
        "cycle+1",
        "cycle=1",
        "a=cycle",

        // -------------------------------------------------------
        // Exit
     R"(do i = 1, 5
            exit
        end do)",
     R"(do
            exit
        enddo)",
     R"(do while (x)
            exit
        end do)",
     R"(do i = 1, 5
            exit = 5
        end do)",
        "exit",
        "exit+1",
        "exit=1",
        "a=exit",

        // -------------------------------------------------------
        // If
        R"(subroutine g
    if (x) then
        a = 5
        b = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    end   If
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    ENDIF
    end subroutine)",
        R"(subroutine g
    if (x) then
        endif = 5
    ENDIF
    end subroutine)",
        R"(subroutine g
    if (else) then
        a = 5
        b = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (else) then
        then = 5
        else = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    else
        b = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
    else
        b = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    else
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
    else
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
        c = 7
    else
        b = 4
        e = 5
    end if
    end subroutine)",
        R"(subroutine g
    if (else) then
        else = 5
    else
        else = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    else if (z) then
        c = 3
    end if
    end subroutine)",
        R"(subroutine g
    if (else) then
        else = 5
    else if (else) then
    else if (else) then
    else if (else) then
        else = 3
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
    else if (y) then
        b = 4
    else
        c = 3
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
        if (y) then
            b = 4
        else
            c = 3
        end if
    end if
    end subroutine)",
        R"(subroutine g
    if (x) then
        a = 5
        if (y) then
            b = 4
        end if
    else
        c = 3
    end if
    end subroutine)",
        R"(if (x) then
        a = 5
        if (y) then
            b = 4
        end if
    else
        c = 3
    end if)",
     R"(subroutine a
if (a) then
    x = 1
else
    x = 2
end if
end subroutine
)",
     R"(subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
end if
end subroutine
)",
    R"(subroutine a
if (a) then
    x = 1
else if (b) then
    x = 2
else if (c) then
    x = 2
end if
end subroutine
)",
    R"(subroutine a
if (a) then
    x = 1
end if
end subroutine
)",
    R"(subroutine a
if (a) then
    x = 1
    y = 2
end if
end subroutine
)",
    R"(subroutine a
if (a) x = 1
end subroutine
)",
    "if (a) x = 1",
        /*
    R"(subroutine a
if (a) &
    x = 1
end subroutine
)",
    R"(
subroutine a
if (a) &     ! if statement
    x = 1
end subroutine
)",
*/

        // -------------------------------------------------------
        // Program
        R"(program g
    x = y
    x = 2*y
    end program)",
        R"(program g
    end program)",
        R"(program g
    x = y; ;


    x = 2*y;; ;

    end program)",
        "program g; x = y; x = 2*y; end program",
        R"(program f
    subroutine = y
    x = 2*subroutine
    end program)",
        R"(program g
    x = y
    end program g)",
        R"(PROGRAM TESTFortran90
      integer stop ; stop = 1 ; do while ( stop .eq. 0 ) ; end do
      END PROGRAM TESTFortran90)",

        // -------------------------------------------------------
        // Function
        R"(function g()
    x = y
    x = 2*y
    end function)",
        R"(function g()
    end function)",
        R"(function g()
    x = y; ;


    x = 2*y;; ;

    end function)",
        "function g(); x = y; x = 2*y; end function",
        R"(function f()
    subroutine = y
    x = 2*subroutine
    end function)",
        R"(function f()
integer :: f
f = 5
end function)",
        R"(real(dp) function f()
        f = 1
        end function
        )",
        R"(real(dp) function f(e)
        f = 1
        end function
        )",
        R"(real(dp) function f(e, b)
        f = 1
        end function
        )",
        R"(real(dp) pure function f(e, b)
        f = 1
        end function
        )",
        R"(real(dp) pure function f(e) result(r)
        r = 1
        end function
        )",
        R"(real(dp) recursive function f(e) result(r)
        r = 1
        end function
        )",
        R"(function f(e)
        f = 1
        end function
        )",
        R"(function f(e)
        f = 1
        end function)",

        // -------------------------------------------------------
        // Subroutine
        R"(subroutine g
    x = y
    x = 2*y
    end subroutine)",
        R"(subroutine g
    end subroutine)",
        R"(subroutine g
    x = y

    x = 2*y
    end subroutine)",
        R"(subroutine g

    x = y


    x = 2*y



    end subroutine)",
        R"(subroutine g
    x = y
    ;;;;;; ; ; ;
    x = 2*y
    end subroutine)",
        R"(subroutine g
    x = y;
    x = 2*y;
    end subroutine)",
        R"(subroutine g
    x = y; ;
    x = 2*y;; ;
    end subroutine)",
        "subroutine g; x = y; x = 2*y; end subroutine",
        R"(subroutine f
    subroutine = y
    x = 2*subroutine
    end subroutine)",
        R"(subroutine f
        integer :: x
        end subroutine
        )",
        /*
        R"(subroutine f()
        ! Some comment
        integer :: x
        ! Some other comment
        end subroutine
        )",
        */
        R"(subroutine f()
        integer :: x
        end subroutine)",
        R"(subroutine f(a, b, c, d)
        integer, intent(in) :: a, b
        integer, intent ( in ) :: c, d
        integer :: z
        integer::y
        end subroutine
        )",
        R"(subroutine f(a, b, c, d)
        integer, intent(out) :: a, b
        integer, intent(inout) :: c, d
        integer :: z
        integer::y
        end subroutine
        )",

        R"(subroutine saxpy(n, a, x, y)
        real(dp), intent(in) :: x(:), a
        real(dp), intent(inout) :: y(:)
        integer, intent(in) :: n
        integer :: i
        do i = 1, n
            y(i) = a*x(i)+y(i)
        enddo
        end subroutine saxpy)",

        // -------------------------------------------------------
        // Use
        "use b",
        "use a, only: b, c",
        "use a, only: x => b, c, d => a",

        // -------------------------------------------------------
        // Program
        R"(program test
        implicit none
        integer :: x
        x = 1
        call a(x)
        end program
        )",

        R"(program test
        implicit none
        integer :: x
        x = 1
        call a(x)
        end program)",

        R"(program test
        implicit none
        integer :: x
        x = 1
        call a(x)
        contains
            subroutine a(b)
            integer, intent(in) :: b
            end subroutine
        end program
        )",

        R"(program test
        implicit none
        integer :: x
        x = 1
        call a(x)
        contains
            subroutine a(b)
            integer, intent(in) :: b
            end subroutine

            subroutine f(b)
            integer, intent(in) :: b
            end subroutine

            integer function a() result(r)
            r = 5
            end function

        end program
        )",

        // -------------------------------------------------------
        // Module

        R"(module test
        implicit none
        integer :: x
        end module
        )",

        R"(module test
        implicit none
        integer :: x
        end module)",

        R"(module test
        implicit none
        private
        integer :: x
        end module
        )",

        R"(module test
        implicit none
        private x, y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        private :: x, y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        public
        integer :: x
        end module
        )",

        R"(module test
        implicit none
        public x, y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        public :: x, y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        private
        public :: x, y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        private x
        public y
        integer :: x, y
        end module
        )",

        R"(module test
        implicit none
        contains
            subroutine a(b)
            integer, intent(in) :: b
            end subroutine
        end module
        )",

        R"(module test
        implicit none
        contains
            subroutine a(b)
            integer, intent(in) :: b
            end subroutine

            subroutine f(b)
            integer, intent(in) :: b
            end subroutine
        end module
        )",

        R"(module test
        implicit none
        integer :: x
        contains
            subroutine a(b)
            integer, intent(in) :: b
            end subroutine
        end module
        )",

        R"(module test
        implicit none
        contains
            function f() result(y)
            y = 0
            end function
        end module
        )",

        R"(module test
        implicit none
        interface name
            module procedure a
            module procedure b
            module procedure c
        end interface
        end module
        )",


        // -------------------------------------------------------
        // Case sensitivity
        "Integer :: x",
        "INTEGER :: x",
        "iNteger :: x",
        "integeR, dImenSion(9,10) :: c",
        "integer, DIMENSION(:,:), intenT(In) :: e",
        "integer(c_Int) :: i",

        "Real a",
        "REAL :: a",
        "real(dp), aLLocatable :: a(:,:)",
        "cHAracter(len=*) :: c",

        "tYPe(xx), inTEnt(inOUt) :: x, y",

        // -------------------------------------------------------
        // Interactivity
        "a = 5; b = 6",

        R"(a = 5
        b = 6
        )",

        R"(a = 5
        print *, a
        )",

        R"(a = 5

        subroutine p()
        print *, a
        end subroutine

        call p()
        )",

        /*
        R"(module a
        implicit none
        integer :: i
        end module

        use a, only: i

        i = 5
        )",
        */

        R"(use a, only: i

        i = 5
        )",

        R"(use a, only: i
        i = 5
        )",

        /*
        R"(!x = [1, 2, 3]
        !y = [1, 2, 1]
        call plot(x, y, "o-")
        )",
        */

        R"(x = [1, 2, 3]
        y = [1, 2, 1]
        call plot(x, y, "o-")
        )",



    };
    std::vector<std::string> o;
    for (std::string &s: v) {
        INFO(s);
        o.push_back(P(s));
    }
    {
        std::ofstream out;
        out.open("ref_pickle.txt.new");
        for (std::string &s: o) {
            out << s << std::endl;
        }
    }
    std::vector<std::string> ref;
    {
        std::ifstream in;
        in.open("ref_pickle.txt");
        std::string s;
        std::getline(in, s);
        while (in.rdstate() == std::ios_base::goodbit) {
            ref.push_back(s);
            std::getline(in, s);
        }

    }
    REQUIRE(o.size() == ref.size());
    for (size_t i=0; i<o.size(); i++) {
        CHECK(o[i] == ref[i]);
    }
}
