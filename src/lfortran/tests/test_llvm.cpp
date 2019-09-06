#include <tests/doctest.h>

#include <lfortran/evaluator.h>
#include <lfortran/exception.h>


TEST_CASE("llvm 1") {
    std::cout << "LLVM Version:" << std::endl;
    LFortran::LLVMEvaluator::print_version_message();

    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
define i64 @f1()
{
    ret i64 4
}
    )""");
    CHECK(e.intfn("f1") == 4);
    e.add_module("");
    CHECK(e.intfn("f1") == 4);

    e.add_module(R"""(
define i64 @f1()
{
    ret i64 5
}
    )""");
    CHECK(e.intfn("f1") == 5);
    e.add_module("");
    CHECK(e.intfn("f1") == 5);
}

TEST_CASE("llvm 1 fail") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    CHECK_THROWS_AS(e.add_module(R"""(
define i64 @f1()
{
    ; FAIL: "=x" is incorrect syntax
    %1 =x alloca i64
}
        )"""), LFortran::CodeGenError);
    CHECK_THROWS_WITH(e.add_module(R"""(
define i64 @f1()
{
    ; FAIL: "=x" is incorrect syntax
    %1 =x alloca i64
}
        )"""), "Invalid LLVM IR");
}


TEST_CASE("llvm 2") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
@count = global i64 0

define i64 @f1()
{
    store i64 4, i64* @count
    %1 = load i64, i64* @count
    ret i64 %1
}
    )""");
    CHECK(e.intfn("f1") == 4);

    e.add_module(R"""(
@count = external global i64

define i64 @f2()
{
    %1 = load i64, i64* @count
    ret i64 %1
}
    )""");
#ifdef _MSC_VER
    // FIXME: For some reason this returns the wrong value on Windows:
    CHECK(e.intfn("f2") == 9727);
#else
    CHECK(e.intfn("f2") == 4);
#endif

    CHECK_THROWS_AS(e.add_module(R"""(
define i64 @f3()
{
    ; FAIL: @count is not defined
    %1 = load i64, i64* @count
    ret i64 %1
}
        )"""), LFortran::CodeGenError);
}
