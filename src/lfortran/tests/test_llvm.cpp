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

TEST_CASE("llvm 3") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
@count = global i64 5
    )""");

    e.add_module(R"""(
@count = external global i64

define i64 @f1()
{
    %1 = load i64, i64* @count
    ret i64 %1
}

define void @inc()
{
    %1 = load i64, i64* @count
    %2 = add i64 %1, 1
    store i64 %2, i64* @count
    ret void
}
    )""");
#ifdef _MSC_VER
    // FIXME: For some reason this returns the wrong value on Windows:
    CHECK(e.intfn("f1") == 9727);
#else
    CHECK(e.intfn("f1") == 5);
#endif
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 6);
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 7);

    e.add_module(R"""(
@count = external global i64

define void @inc2()
{
    %1 = load i64, i64* @count
    %2 = add i64 %1, 2
    store i64 %2, i64* @count
    ret void
}
    )""");
    CHECK(e.intfn("f1") == 7);
    e.voidfn("inc2");
    CHECK(e.intfn("f1") == 9);
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 10);
    e.voidfn("inc2");
    CHECK(e.intfn("f1") == 12);

    // Test that we can have another independent LLVMEvaluator and use both at
    // the same time:
    LFortran::LLVMEvaluator e2 = LFortran::LLVMEvaluator();
    e2.add_module(R"""(
@count = global i64 5

define i64 @f1()
{
    %1 = load i64, i64* @count
    ret i64 %1
}

define void @inc()
{
    %1 = load i64, i64* @count
    %2 = add i64 %1, 1
    store i64 %2, i64* @count
    ret void
}
    )""");

    CHECK(e2.intfn("f1") == 5);
    e2.voidfn("inc");
    CHECK(e2.intfn("f1") == 6);
    e2.voidfn("inc");
    CHECK(e2.intfn("f1") == 7);

    CHECK(e.intfn("f1") == 12);
    e2.voidfn("inc");
    CHECK(e2.intfn("f1") == 8);
    CHECK(e.intfn("f1") == 12);
    e.voidfn("inc");
    CHECK(e2.intfn("f1") == 8);
    CHECK(e.intfn("f1") == 13);

}

TEST_CASE("llvm 4") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
@count = global i64 5

define i64 @f1()
{
    %1 = load i64, i64* @count
    ret i64 %1
}

define void @inc()
{
    %1 = load i64, i64* @count
    %2 = add i64 %1, 1
    store i64 %2, i64* @count
    ret void
}
)""");
    CHECK(e.intfn("f1") == 5);
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 6);
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 7);

    e.add_module(R"""(
declare void @inc()

define void @inc2()
{
    call void @inc()
    call void @inc()
    ret void
}
)""");
    CHECK(e.intfn("f1") == 7);
    e.voidfn("inc2");
    CHECK(e.intfn("f1") == 9);
    e.voidfn("inc");
    CHECK(e.intfn("f1") == 10);
    e.voidfn("inc2");
    CHECK(e.intfn("f1") == 12);

    CHECK_THROWS_AS(e.add_module(R"""(
define void @inc2()
{
    ; FAIL: @inc is not defined
    call void @inc()
    call void @inc()
    ret void
}
        )"""), LFortran::CodeGenError);
}
