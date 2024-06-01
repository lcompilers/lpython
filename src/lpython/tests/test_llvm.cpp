#include <tests/doctest.h>

#include <cmath>

#include <lpython/python_evaluator.h>
#include <libasr/codegen/evaluator.h>
#include <libasr/exception.h>
#include <libasr/asr.h>
#include <libasr/codegen/asr_to_llvm.h>
#include <lpython/pickle.h>

using LCompilers::TRY;
using LCompilers::PythonCompiler;
using LCompilers::CompilerOptions;


TEST_CASE("llvm 1") {
    //std::cout << "LLVM Version:" << std::endl;
    //LCompilers::LLVMEvaluator::print_version_message();

    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
define i64 @f1()
{
    ret i64 4
}
    )""");
    CHECK(e.int64fn("f1") == 4);
    e.add_module("");
    //CHECK(e.int64fn("f1") == 4);

    e.add_module(R"""(
define i64 @f2()
{
    ret i64 5
}
    )""");
    CHECK(e.int64fn("f2") == 5);
    //e.add_module("");
    //CHECK(e.int64fn("f2") == 5);
}

TEST_CASE("llvm 1 fail") {
    LCompilers::LLVMEvaluator e;
    CHECK_THROWS_AS(e.add_module(R"""(
define i64 @f1()
{
    ; FAIL: "=x" is incorrect syntax
    %1 =x alloca i64
}
        )"""), LCompilers::LCompilersException);
    CHECK_THROWS_WITH(e.add_module(R"""(
define i64 @f1()
{
    ; FAIL: "=x" is incorrect syntax
    %1 =x alloca i64
}
        )"""), "parse_module(): Invalid LLVM IR");
}


TEST_CASE("llvm 2") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
@count = global i64 0

define i64 @f1()
{
    store i64 4, i64* @count
    %1 = load i64, i64* @count
    ret i64 %1
}
    )""");
    CHECK(e.int64fn("f1") == 4);

    e.add_module(R"""(
@count = external global i64

define i64 @f2()
{
    %1 = load i64, i64* @count
    ret i64 %1
}
    )""");
    CHECK(e.int64fn("f2") == 4);

    CHECK_THROWS_AS(e.add_module(R"""(
define i64 @f3()
{
    ; FAIL: @count is not defined
    %1 = load i64, i64* @count
    ret i64 %1
}
        )"""), LCompilers::LCompilersException);
}

TEST_CASE("llvm 3") {
    LCompilers::LLVMEvaluator e;
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
    CHECK(e.int64fn("f1") == 5);
    /*
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 6);
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 7);
    */

    /*
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
    CHECK(e.int64fn("f1") == 7);
    e.voidfn("inc2");
    CHECK(e.int64fn("f1") == 9);
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 10);
    e.voidfn("inc2");
    CHECK(e.int64fn("f1") == 12);
    */

    // Test that we can have another independent LLVMEvaluator and use both at
    // the same time:
    /*
    LCompilers::LLVMEvaluator e2;
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

    CHECK(e2.int64fn("f1") == 5);
    e2.voidfn("inc");
    CHECK(e2.int64fn("f1") == 6);
    e2.voidfn("inc");
    CHECK(e2.int64fn("f1") == 7);

    CHECK(e.int64fn("f1") == 12);
    e2.voidfn("inc");
    CHECK(e2.int64fn("f1") == 8);
    CHECK(e.int64fn("f1") == 12);
    e.voidfn("inc");
    CHECK(e2.int64fn("f1") == 8);
    CHECK(e.int64fn("f1") == 13);
*/
}

TEST_CASE("llvm 4") {
    LCompilers::LLVMEvaluator e;
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
    CHECK(e.int64fn("f1") == 5);
    /*
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 6);
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 7);

    e.add_module(R"""(
declare void @inc()

define void @inc2()
{
    call void @inc()
    call void @inc()
    ret void
}
)""");
    CHECK(e.int64fn("f1") == 7);
    e.voidfn("inc2");
    CHECK(e.int64fn("f1") == 9);
    e.voidfn("inc");
    CHECK(e.int64fn("f1") == 10);
    e.voidfn("inc2");
    CHECK(e.int64fn("f1") == 12);

    CHECK_THROWS_AS(e.add_module(R"""(
define void @inc2()
{
    ; FAIL: @inc is not defined
    call void @inc()
    call void @inc()
    ret void
}
        )"""), LCompilers::LCompilersException);
	*/
}

TEST_CASE("llvm array 1") {
    LCompilers::LLVMEvaluator e;
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
    CHECK(e.int64fn("f") == 6);
}

TEST_CASE("llvm array 2") {
    LCompilers::LLVMEvaluator e;
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
    //CHECK(e.int64fn("f") == 6);
}

int f(int a, int b) {
    return a+b;
}

TEST_CASE("llvm callback 0") {
    LCompilers::LLVMEvaluator e;
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
    CHECK(e.int64fn("f1") == 5);
}



// Tests passing the complex struct by reference
TEST_CASE("llvm complex type") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
%complex = type { float, float }

define float @sum2(%complex* %a)
{
    %a1addr = getelementptr %complex, %complex* %a, i32 0, i32 0
    %a1 = load float, float* %a1addr

    %a2addr = getelementptr %complex, %complex* %a, i32 0, i32 1
    %a2 = load float, float* %a2addr

    %r = fadd float %a2, %a1

    ret float %r
}

define float @f()
{
    %a = alloca %complex

    %a1 = getelementptr %complex, %complex* %a, i32 0, i32 0
    store float 3.5, float* %a1

    %a2 = getelementptr %complex, %complex* %a, i32 0, i32 1
    store float 4.5, float* %a2

    %r = call float @sum2(%complex* %a)
    ret float %r
}
    )""");
    CHECK(std::abs(e.floatfn("f") - 8) < 1e-6);
}

// Tests passing the complex struct by value
TEST_CASE("llvm complex type value") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
%complex = type { float, float }

define float @sum2(%complex %a_value)
{
    %a = alloca %complex
    store %complex %a_value, %complex* %a

    %a1addr = getelementptr %complex, %complex* %a, i32 0, i32 0
    %a1 = load float, float* %a1addr

    %a2addr = getelementptr %complex, %complex* %a, i32 0, i32 1
    %a2 = load float, float* %a2addr

    %r = fadd float %a2, %a1
    ret float %r
}

define float @f()
{
    %a = alloca %complex

    %a1 = getelementptr %complex, %complex* %a, i32 0, i32 0
    store float 3.5, float* %a1

    %a2 = getelementptr %complex, %complex* %a, i32 0, i32 1
    store float 4.5, float* %a2

    %ap = load %complex, %complex* %a
    %r = call float @sum2(%complex %ap)
    ret float %r
}
    )""");
    //CHECK(std::abs(e.floatfn("f") - 8) < 1e-6);
}

// Tests passing boolean by reference
TEST_CASE("llvm boolean type") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(

define i1 @and_func(i1* %p, i1* %q)
{
    %pval = load i1, i1* %p
    %qval = load i1, i1* %q

    %r = and i1 %pval, %qval
    ret i1 %r
}

define i1 @b()
{
    %p = alloca i1
    %q = alloca i1

    store i1 1, i1* %p
    store i1 0, i1* %q

    %r = call i1 @and_func(i1* %p, i1* %q)

    ret i1 %r
}
    )""");
    CHECK(e.boolfn("b") == false);
}

// Tests passing boolean by value
TEST_CASE("llvm boolean type") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(

define i1 @and_func(i1 %p, i1 %q)
{
    %r = and i1 %p, %q
    ret i1 %r
}

define i1 @b()
{
    %p = alloca i1
    %q = alloca i1

    store i1 1, i1* %p
    store i1 0, i1* %q

    %pval = load i1, i1* %p
    %qval = load i1, i1* %q

    %r = call i1 @and_func(i1 %pval, i1 %qval)

    ret i1 %r
}
    )""");
    CHECK(e.boolfn("b") == false);
}

// Tests pointers
TEST_CASE("llvm pointers 1") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
@r = global i64 0

define i64 @f()
{
    store i64 8, i64* @r

    ; Dereferences the pointer %r
    ;%rval = load i64, i64* @r

    %raddr = ptrtoint i64* @r to i64

    ret i64 %raddr
}
    )""");
    int64_t r = e.int64fn("f");
    CHECK(r != 8);
    int64_t *p = (int64_t*)r;
    CHECK(*p == 8);
}

TEST_CASE("llvm pointers 2") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
@r = global float 0.0

define i64 @f()
{
    store float 8.0, float* @r

    %raddr = ptrtoint float* @r to i64

    ret i64 %raddr
}
    )""");
    int64_t r = e.int64fn("f");
    float *p = (float *)r;
    CHECK(std::abs(*p - 8) < 1e-6);
}

TEST_CASE("llvm pointers 3") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
; Takes a variable and returns a pointer to it
define i64 @pointer_reference(float* %var)
{
    %ret = ptrtoint float* %var to i64
    ret i64 %ret
}

; Takes a pointer and returns the value of what it points to
define float @pointer_dereference(i64 %p)
{
    %p2 = inttoptr i64 %p to float*
    %ret = load float, float* %p2
    ret float %ret
}

define float @f()
{
    %var = alloca float
    store float 8.0, float* %var

    %pointer_val = call i64 @pointer_reference(float* %var)

    ; Save the pointer to a variable
    %pointer_var = alloca i64
    store i64 %pointer_val, i64* %pointer_var
    ; Extract value out of the pointer %pointer_var
    %pointer_val2 = load i64, i64* %pointer_var

    %ret = call float @pointer_dereference(i64 %pointer_val2)

    ret float %ret
}
    )""");
    float r = e.floatfn("f");
    CHECK(std::abs(r - 8) < 1e-6);
}

TEST_CASE("llvm pointers 4") {
    LCompilers::LLVMEvaluator e;
    e.add_module(R"""(
; Takes a variable and returns a pointer to it
define float* @pointer_reference(float* %var)
{
    ret float* %var
}

define float @pointer_dereference(float* %var)
{
    %ret = load float, float* %var
    ret float %ret
}

define float @f()
{
    %var = alloca float
    store float 8.0, float* %var

    %pointer_val = call float* @pointer_reference(float* %var)

    ; Save the pointer to a variable
    %pointer_var = alloca float*
    store float* %pointer_val, float** %pointer_var
    ; Extract value out of the pointer %pointer_var
    %pointer_val2 = load float*, float** %pointer_var

    %ret = call float @pointer_dereference(float* %pointer_val2)

    ret float %ret
}
    )""");
    float r = e.floatfn("f");
    CHECK(std::abs(r - 8) < 1e-6);
}

TEST_CASE("PythonCompiler 1") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>
    r = e.evaluate2("1");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 1);
}

TEST_CASE("PythonCompiler i32 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("1");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 1);

    r = e.evaluate2("1 + 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 3);

    r = e.evaluate2("1 - 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == -1);

    r = e.evaluate2("1 * 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("3 ** 3");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 27);

    r = e.evaluate2("4 // 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("4 / 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler i32 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: i32");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = 5");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 5);

    r = e.evaluate2("j: i32 = 9");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 9);

    r = e.evaluate2("i + j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 14);
}

TEST_CASE("PythonCompiler i64 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i64(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 1);

    r = e.evaluate2("i64(1) + i64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 3);

    r = e.evaluate2("i64(1) - i64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == -1);

    r = e.evaluate2("i64(1) * i64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 2);

    r = e.evaluate2("i64(3) ** i64(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 27);

    r = e.evaluate2("i64(4) // i64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 2);

    r = e.evaluate2("i64(4) / i64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler i64 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: i64");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = i64(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 5);

    r = e.evaluate2("j: i64 = i64(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 9);

    r = e.evaluate2("i + j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer8);
    CHECK(r.result.i64 == 14);
}

TEST_CASE("PythonCompiler tmp 1") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>
    r = e.evaluate2("3 % 2");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
    CHECK(r.result.i32 == 1);
}

// TEST_CASE("PythonCompiler asr verify 1") {
//     CompilerOptions cu;
//     cu.po.disable_main = true;
//     cu.emit_debug_line_column = false;
//     cu.generate_object_code = false;
//     cu.interactive = true;
//     cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
//     PythonCompiler e(cu);
//     LCompilers::Result<PythonCompiler::EvalResult>
//     r = e.evaluate2("i: i32 = 3 % 2");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::none);
//     r = e.evaluate2("i");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
//     CHECK(r.result.i32 == 1);
// }

// TEST_CASE("PythonCompiler asr verify 2") {
//     CompilerOptions cu;
//     cu.po.disable_main = true;
//     cu.emit_debug_line_column = false;
//     cu.generate_object_code = false;
//     cu.interactive = true;
//     cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
//     std::cout << cu.po.runtime_library_dir << std::endl;
//     PythonCompiler e(cu);
//     LCompilers::Result<PythonCompiler::EvalResult>
//     r = e.evaluate2(R"(
// def is_even(x: i32) -> i32:
//     if x % 2 == 0:
//         return 1
//     return 0
// )");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::none);
//     r = e.evaluate2("is_even(4)");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
//     CHECK(r.result.i32 == 1);
//     r = e.evaluate2("is_even(3)");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
//     CHECK(r.result.i32 == 0);
// }

// TEST_CASE("PythonCompiler asr verify 3") {
//     CompilerOptions cu;
//     cu.po.disable_main = true;
//     cu.emit_debug_line_column = false;
//     cu.generate_object_code = false;
//     cu.interactive = true;
//     cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
//     PythonCompiler e(cu);
//     LCompilers::Result<PythonCompiler::EvalResult>
//     r = e.evaluate2(R"(
// def addi(x: i32, y: i32) -> i32:
//     return x + y
// )");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::none);
//     r = e.evaluate2(R"(
// def subi(x: i32, y: i32) -> i32:
//     return addi(x, -y)
// )");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::none);
//     r = e.evaluate2("addi(2, 3)");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
//     CHECK(r.result.i32 == 5);
//     r = e.evaluate2("subi(2, 3)");
//     CHECK(r.ok);
//     CHECK(r.result.type == PythonCompiler::EvalResult::integer4);
//     CHECK(r.result.i32 == -1);
// }

TEST_CASE("PythonCompiler asr verify 4") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>
    r = e.evaluate2(R"(
def addr(x: f64, y: f64) -> f64:
    return x + y
)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2(R"(
def subr(x: f64, y: f64) -> f64:
    return addr(x, -y)
)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("addr(2.5, 3.5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 6);
    r = e.evaluate2("subr(2.5, 3.5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == -1);
}

TEST_CASE("PythonCompiler u32 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("u32(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 1);

    r = e.evaluate2("u32(1) + u32(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 3);

    r = e.evaluate2("u32(20) - u32(10)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 10);

    r = e.evaluate2("u32(1) * u32(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u32(3) ** u32(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 27);

    r = e.evaluate2("u32(4) // u32(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u32(4) / u32(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler u32 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: u32");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = u32(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 5);

    r = e.evaluate2("j: u32 = u32(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 9);

    r = e.evaluate2("i * j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger4);
    CHECK(r.result.u32 == 45);
}

TEST_CASE("PythonCompiler u64 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("u64(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 1);

    r = e.evaluate2("u64(1) + u64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 3);

    r = e.evaluate2("u64(20) - u64(10)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 10);

    r = e.evaluate2("u64(1) * u64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 2);

    r = e.evaluate2("u64(3) ** u64(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 27);

    r = e.evaluate2("u64(4) // u64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 2);

    r = e.evaluate2("u64(4) / u64(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler u64 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: u64");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = u64(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 5);

    r = e.evaluate2("j: u64 = u64(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 9);

    r = e.evaluate2("i * j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger8);
    CHECK(r.result.u64 == 45);
}

TEST_CASE("PythonCompiler i8 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i8(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 1);

    r = e.evaluate2("i8(1) + i8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 3);

    r = e.evaluate2("i8(1) - i8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == -1);

    r = e.evaluate2("i8(1) * i8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("i8(3) ** i8(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 27);

    r = e.evaluate2("i8(4) // i8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("i8(4) / i8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler i8 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: i8");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = i8(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 5);

    r = e.evaluate2("j: i8 = i8(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 9);

    r = e.evaluate2("i + j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer1);
    CHECK(r.result.i32 == 14);
}

TEST_CASE("PythonCompiler u8 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("u8(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 1);

    r = e.evaluate2("u8(1) + u8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 3);

    r = e.evaluate2("u8(20) - u8(10)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 10);

    r = e.evaluate2("u8(1) * u8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u8(3) ** u8(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 27);

    r = e.evaluate2("u8(4) // u8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u8(4) / u8(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler u8 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: u8");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = u8(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 5);

    r = e.evaluate2("j: u8 = u8(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 9);

    r = e.evaluate2("i * j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger1);
    CHECK(r.result.u32 == 45);
}

TEST_CASE("PythonCompiler i16 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i16(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 1);

    r = e.evaluate2("i16(1) + i16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 3);

    r = e.evaluate2("i16(1) - i16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == -1);

    r = e.evaluate2("i16(1) * i16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("i16(3) ** i16(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 27);

    r = e.evaluate2("i16(4) // i16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 2);

    r = e.evaluate2("i16(4) / i16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler i16 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: i16");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = i16(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 5);

    r = e.evaluate2("j: i16 = i16(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 9);

    r = e.evaluate2("i + j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::integer2);
    CHECK(r.result.i32 == 14);
}

TEST_CASE("PythonCompiler u16 expressions") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("u16(1)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 1);

    r = e.evaluate2("u16(1) + u16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 3);

    r = e.evaluate2("u16(20) - u16(10)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 10);

    r = e.evaluate2("u16(1) * u16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u16(3) ** u16(3)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 27);

    r = e.evaluate2("u16(4) // u16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 2);

    r = e.evaluate2("u16(4) / u16(2)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::real8);
    CHECK(r.result.f64 == 2);
}

TEST_CASE("PythonCompiler u16 declaration") {
    CompilerOptions cu;
    cu.po.disable_main = true;
    cu.emit_debug_line_column = false;
    cu.generate_object_code = false;
    cu.interactive = true;
    cu.po.runtime_library_dir = LCompilers::LPython::get_runtime_library_dir();
    PythonCompiler e(cu);
    LCompilers::Result<PythonCompiler::EvalResult>

    r = e.evaluate2("i: u16");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("i = u16(5)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::statement);
    r = e.evaluate2("i");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 5);

    r = e.evaluate2("j: u16 = u16(9)");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::none);
    r = e.evaluate2("j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 9);

    r = e.evaluate2("i * j");
    CHECK(r.ok);
    CHECK(r.result.type == PythonCompiler::EvalResult::unsignedInteger2);
    CHECK(r.result.u32 == 45);
}
