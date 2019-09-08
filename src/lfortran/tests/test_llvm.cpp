#include <tests/doctest.h>

#include <lfortran/codegen/evaluator.h>
#include <lfortran/exception.h>
#include <lfortran/ast.h>
#include <lfortran/asr.h>
#include <lfortran/parser/parser.h>
#include <lfortran/semantics/ast_to_asr.h>
#include <lfortran/codegen/asr_to_llvm.h>
#include <lfortran/pickle.h>


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
    CHECK(e.intfn("f2") == 4);

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
    CHECK(e.intfn("f1") == 5);
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

TEST_CASE("llvm array 1") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
; Sum the three elements in %a
define i64 @sum3(i64* %a)
{
    %a1addr = getelementptr i64, i64* %a, i64 0
    %a1 = load i64, i64* %a1addr

    %a2addr = getelementptr i64, i64* %a, i64 1
    %a2 = load i64, i64* %a2addr

    %a3addr = getelementptr i64, i64* %a, i64 2
    %a3 = load i64, i64* %a3addr

    %tmp = add i64 %a2, %a1
    %r = add i64 %a3, %tmp

    ret i64 %r
}


define i64 @f()
{
    %a = alloca [3 x i64]

    %a1 = getelementptr [3 x i64], [3 x i64]* %a, i64 0, i64 0
    store i64 1, i64* %a1

    %a2 = getelementptr [3 x i64], [3 x i64]* %a, i64 0, i64 1
    store i64 2, i64* %a2

    %a3 = getelementptr [3 x i64], [3 x i64]* %a, i64 0, i64 2
    store i64 3, i64* %a3

    %r = call i64 @sum3(i64* %a1)
    ret i64 %r
}
    )""");
    CHECK(e.intfn("f") == 6);
}

TEST_CASE("llvm array 2") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    e.add_module(R"""(
%array = type {i64, [3 x i64]}

; Sum the three elements in %a
define i64 @sum3(%array* %a)
{
    %a1addr = getelementptr %array, %array* %a, i64 0, i32 1, i64 0
    %a1 = load i64, i64* %a1addr

    %a2addr = getelementptr %array, %array* %a, i64 0, i32 1, i64 1
    %a2 = load i64, i64* %a2addr

    %a3addr = getelementptr %array, %array* %a, i64 0, i32 1, i64 2
    %a3 = load i64, i64* %a3addr

    %tmp = add i64 %a2, %a1
    %r = add i64 %a3, %tmp

    ret i64 %r
}


define i64 @f()
{
    %a = alloca %array

    %idx0 = getelementptr %array, %array* %a, i64 0, i32 0
    store i64 3, i64* %idx0

    %idx1 = getelementptr %array, %array* %a, i64 0, i32 1, i64 0
    store i64 1, i64* %idx1
    %idx2 = getelementptr %array, %array* %a, i64 0, i32 1, i64 1
    store i64 2, i64* %idx2
    %idx3 = getelementptr %array, %array* %a, i64 0, i32 1, i64 2
    store i64 3, i64* %idx3

    %r = call i64 @sum3(%array* %a)
    ret i64 %r
}
    )""");
    CHECK(e.intfn("f") == 6);
}

int f(int a, int b) {
    return a+b;
}

TEST_CASE("llvm callback 0") {
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    std::string addr = std::to_string((int64_t)f);
    e.add_module(R"""(
define i64 @addrcaller(i64 %a, i64 %b)
{
    %f = inttoptr i64 )""" + addr + R"""( to i64 (i64, i64)*
    %r = call i64 %f(i64 %a, i64 %b)
    ret i64 %r
}

define i64 @f1()
{
    %r = call i64 @addrcaller(i64 2, i64 3)
    ret i64 %r
}
    )""");
    CHECK(e.intfn("f1") == 5);
}


TEST_CASE("ASR -> LLVM 1") {
    std::string source = R"(function f()
integer :: f
f = 5
end function)";

    // Src -> AST
    Allocator al(4*1024);
    LFortran::AST::ast_t* ast = LFortran::parse2(al, source);
    CHECK(LFortran::pickle(*ast) == "(fn f [] () () () [] [(decl [(f integer [] [] ())])] [(= f 5)] [])");

    // AST -> ASR
    LFortran::ASR::asr_t* asr = LFortran::ast_to_asr(al, *ast);
    CHECK(LFortran::pickle(*asr) == "(fn f [] [] () (variable f () Unimplemented (integer Unimplemented [])) () Unimplemented)");

    // ASR -> LLVM
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    std::unique_ptr<LFortran::LLVMModule> m = LFortran::asr_to_llvm(*asr,
            e.get_context());
    std::cout << "Module:" << std::endl;
    std::cout << m->str() << std::endl;

    // LLVM -> Machine code -> Execution
    e.add_module(std::move(m));
    CHECK(e.intfn("f") == 5);
}

TEST_CASE("ASR -> LLVM 2") {
    std::string source = R"(function f()
integer :: f
f = 4
end function)";

    // Src -> AST
    Allocator al(4*1024);
    LFortran::AST::ast_t* ast = LFortran::parse2(al, source);
    CHECK(LFortran::pickle(*ast) == "(fn f [] () () () [] [(decl [(f integer [] [] ())])] [(= f 4)] [])");

    // AST -> ASR
    LFortran::ASR::asr_t* asr = LFortran::ast_to_asr(al, *ast);
    CHECK(LFortran::pickle(*asr) == "(fn f [] [] () (variable f () Unimplemented (integer Unimplemented [])) () Unimplemented)");

    // ASR -> LLVM
    LFortran::LLVMEvaluator e = LFortran::LLVMEvaluator();
    std::unique_ptr<LFortran::LLVMModule> m = LFortran::asr_to_llvm(*asr,
            e.get_context());
    std::cout << "Module:" << std::endl;
    std::cout << m->str() << std::endl;

    // LLVM -> Machine code -> Execution
    e.add_module(std::move(m));
    CHECK(e.intfn("f") == 4);
}
