#include <tests/doctest.h>
#include <iostream>

#include <lfortran/bwriter.h>
#include <lfortran/serialization.h>
#include <lfortran/modfile.h>
#include <lfortran/pickle.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/asr_utils.h>
#include <lfortran/asr_verify.h>

using LFortran::string_to_uint64;
using LFortran::uint64_to_string;
using LFortran::string_to_uint32;
using LFortran::uint32_to_string;

TEST_CASE("Integer conversion") {
    uint64_t i;
    i = 1;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 150;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 256;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 65537;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 16777217;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967295LU;
    CHECK(string_to_uint32(uint32_to_string(i)) == i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 4294967296LU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);

    i = 18446744073709551615LLU;
    CHECK(string_to_uint32(uint32_to_string(i)) != i);
    CHECK(string_to_uint64(uint64_to_string(i)) == i);
}

void ast_ser(const std::string &src) {
    Allocator al(4*1024);

    LFortran::AST::TranslationUnit_t* result;
    result = LFortran::parse2(al, src);
    std::string ast_orig = LFortran::pickle(*result);
    std::string binary = LFortran::serialize(*result);

    LFortran::AST::ast_t *ast;
    ast = LFortran::deserialize_ast(al, binary);
    CHECK(LFortran::AST::is_a<LFortran::AST::unit_t>(*ast));

    std::string ast_new = LFortran::pickle(*ast);

    CHECK(ast_orig == ast_new);
}

void asr_ser(const std::string &src) {
    Allocator al(4*1024);

    LFortran::AST::TranslationUnit_t* ast0;
    ast0 = LFortran::parse2(al, src);
    LFortran::ASR::TranslationUnit_t* asr = LFortran::ast_to_asr(al, *ast0);

    std::string asr_orig = LFortran::pickle(*asr);
    std::string binary = LFortran::serialize(*asr);

    LFortran::ASR::asr_t *asr_new0;
    LFortran::SymbolTable symtab(nullptr);
    asr_new0 = LFortran::deserialize_asr(al, binary, true, symtab);
    CHECK(LFortran::ASR::is_a<LFortran::ASR::unit_t>(*asr_new0));
    LFortran::ASR::TranslationUnit_t *tu
        = LFortran::ASR::down_cast2<LFortran::ASR::TranslationUnit_t>(asr_new0);
    fix_external_symbols(*tu, symtab);
    LFORTRAN_ASSERT(LFortran::asr_verify(*tu));

    std::string asr_new = LFortran::pickle(*asr_new0);

    CHECK(asr_orig == asr_new);
}

void asr_mod(const std::string &src) {
    Allocator al(4*1024);

    LFortran::AST::TranslationUnit_t* ast0;
    ast0 = LFortran::parse2(al, src);
    LFortran::ASR::TranslationUnit_t* asr = LFortran::ast_to_asr(al, *ast0);

    std::string modfile = LFortran::save_modfile(*asr);
    LFortran::SymbolTable symtab(nullptr);
    LFortran::ASR::TranslationUnit_t *asr2 = LFortran::load_modfile(al,
            modfile, true, symtab);
    fix_external_symbols(*asr2, symtab);
    LFORTRAN_ASSERT(LFortran::asr_verify(*asr2));

    CHECK(LFortran::pickle(*asr) == LFortran::pickle(*asr2));
}

TEST_CASE("AST Tests") {
    ast_ser("x = 2+2**2");

    ast_ser(R"""(
x = 2+2**2
)""");

    ast_ser("r = a - 2*x");

    ast_ser("r = f(a) - 2*x");

    ast_ser(R"""(
program expr2
implicit none
integer :: x
x = (2+3)*5
print *, x
end program
)""");

    ast_ser(R"""(
integer function f(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
)""");

    ast_ser(R"""(
module modules_05_mod
implicit none
! TODO: the following line does not work yet
!private
integer, parameter, public :: a = 5
integer, parameter :: b = 6
integer, parameter :: c = 7
public :: c
end module


program modules_05
use modules_05_mod, only: a, c
print *, a, c
end program
)""");

    ast_ser(R"""(
program doconcurrentloop_01
implicit none
real, dimension(10000) :: a, b, c
real :: scalar
integer :: i, nsize
scalar = 10
nsize = size(a)
do concurrent (i = 1:nsize)
    a(i) = 5
    b(i) = 5
end do
call triad(a, b, scalar, c)
print *, "End Stream Triad"

contains

    subroutine triad(a, b, scalar, c)
    real, intent(in) :: a(:), b(:), scalar
    real, intent(out) :: c(:)
    integer :: N, i
    N = size(a)
    do concurrent (i = 1:N)
        c(i) = a(i) + scalar * b(i)
    end do
    end subroutine

end program
)""");

}

TEST_CASE("ASR Tests") {
    asr_ser(R"""(
program expr2
implicit none
integer :: x
x = (2+3)*5
print *, x
end program
)""");

    asr_ser(R"""(
integer function f(a, b) result(r)
integer, intent(in) :: a, b
r = a + b
end function
)""");

    asr_ser(R"""(
program doconcurrentloop_01
implicit none
real, dimension(10000) :: a, b, c
real :: scalar
integer :: i, nsize
scalar = 10
nsize = size(a)
do concurrent (i = 1:nsize)
    a(i) = 5
    b(i) = 5
end do
call triad(a, b, scalar, c)
print *, "End Stream Triad"

contains

    subroutine triad(a, b, scalar, c)
    real, intent(in) :: a(:), b(:), scalar
    real, intent(out) :: c(:)
    integer :: N, i
    N = size(a)
    do concurrent (i = 1:N)
        c(i) = a(i) + scalar * b(i)
    end do
    end subroutine

end program
)""");

    asr_ser(R"""(
module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

program modules_01
use a, only: b
implicit none

call b()

end
)""");

    asr_ser(R"""(
program derived_types_03
implicit none

type :: X
    integer :: i
end type

type(X) :: b

contains

    subroutine Y()
    type :: A
        integer :: i
    end type
    type(A) :: b
    end subroutine

    integer function Z()
    type :: A
        integer :: i
    end type
    type(A) :: b
    Z = 5
    end function

end
)""");

}

TEST_CASE("ASR modfile handling") {
    asr_mod(R"""(
module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module
)""");

}

TEST_CASE("Topological sorting int") {
    std::map<int, std::vector<int>> deps;
    // 1 depends on 2
    deps[1].push_back(2);
    // 3 depends on 1, etc.
    deps[3].push_back(1);
    deps[2].push_back(4);
    deps[3].push_back(4);
    CHECK(LFortran::order_deps(deps) == std::vector<int>({4, 2, 1, 3}));

    deps.clear();
    deps[1].push_back(2);
    deps[1].push_back(3);
    deps[2].push_back(4);
    deps[3].push_back(4);
    CHECK(LFortran::order_deps(deps) == std::vector<int>({4, 2, 3, 1}));

    deps.clear();
    deps[1].push_back(2);
    deps[3].push_back(1);
    deps[3].push_back(4);
    deps[4].push_back(1);
    CHECK(LFortran::order_deps(deps) == std::vector<int>({2, 1, 4, 3}));
}

TEST_CASE("Topological sorting string") {
    std::map<std::string, std::vector<std::string>> deps;
    // A depends on B
    deps["A"].push_back("B");
    // C depends on A, etc.
    deps["C"].push_back("A");
    deps["B"].push_back("D");
    deps["C"].push_back("D");
    CHECK(LFortran::order_deps(deps) == std::vector<std::string>(
                {"D", "B", "A", "C"}));

    deps.clear();
    deps["module_a"].push_back("module_b");
    deps["module_c"].push_back("module_a");
    deps["module_c"].push_back("module_d");
    deps["module_d"].push_back("module_a");
    CHECK(LFortran::order_deps(deps) == std::vector<std::string>(
                {"module_b", "module_a", "module_d", "module_c"}));
}
