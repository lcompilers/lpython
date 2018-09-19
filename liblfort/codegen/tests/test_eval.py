import pytest

from liblfort.codegen.evaluator import FortranEvaluator, LLVMEvaluator

# LLVM

def test_llvm_eval1():
    e = LLVMEvaluator()
    e.add_module("""\
define i64 @f1()
{
  ret i64 4
}
""")
    assert e.intfn("f1") == 4
    e.add_module("")
    assert e.intfn("f1") == 4

    e.add_module("""\
define i64 @f1()
{
  ret i64 5
}
""")
    assert e.intfn("f1") == 5
    e.add_module("")
    assert e.intfn("f1") == 5

def test_llvm_eval1_fail():
    e = LLVMEvaluator()
    with pytest.raises(RuntimeError):
        e.add_module("""\
define i64 @f1()
{
  ; FAIL: "=x" is incorrect syntax
  %1 =x alloca i64
}
""")

def test_llvm_eval2():
    e = LLVMEvaluator()
    e.add_module("""\
@count = global i64 0

define i64 @f1()
{
  store i64 4, i64* @count
  %1 = load i64, i64* @count
  ret i64 %1
}
""")
    assert e.intfn("f1") == 4

    e.add_module("""\
@count = external global i64

define i64 @f2()
{
  %1 = load i64, i64* @count
  ret i64 %1
}
""")
    assert e.intfn("f2") == 4

    with pytest.raises(RuntimeError):
        e.add_module("""\
define i64 @f3()
{
  ; FAIL: @count is not defined
  %1 = load i64, i64* @count
  ret i64 %1
}
""")

def test_llvm_eval3():
    e = LLVMEvaluator()
    e.add_module("""\
@count = global i64 5
""")

    e.add_module("""\
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
""")
    assert e.intfn("f1") == 5
    e.voidfn("inc")
    assert e.intfn("f1") == 6
    e.voidfn("inc")
    assert e.intfn("f1") == 7

    e.add_module("""\
@count = external global i64

define void @inc2()
{
  %1 = load i64, i64* @count
  %2 = add i64 %1, 2
  store i64 %2, i64* @count
  ret void
}
""")
    assert e.intfn("f1") == 7
    e.voidfn("inc2")
    assert e.intfn("f1") == 9
    e.voidfn("inc")
    assert e.intfn("f1") == 10
    e.voidfn("inc2")
    assert e.intfn("f1") == 12

    # Test that we can have another independent LLVMEvaluator and use both at
    # the same time:
    e2 = LLVMEvaluator()
    e2.add_module("""\
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
""")
    assert e2.intfn("f1") == 5
    e2.voidfn("inc")
    assert e2.intfn("f1") == 6
    e2.voidfn("inc")
    assert e2.intfn("f1") == 7

    assert e.intfn("f1") == 12
    e2.voidfn("inc")
    assert e2.intfn("f1") == 8
    assert e.intfn("f1") == 12
    e.voidfn("inc")
    assert e2.intfn("f1") == 8
    assert e.intfn("f1") == 13

def test_llvm_eval4():
    e = LLVMEvaluator()
    e.add_module("""\
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
""")
    assert e.intfn("f1") == 5
    e.voidfn("inc")
    assert e.intfn("f1") == 6
    e.voidfn("inc")
    assert e.intfn("f1") == 7

    e.add_module("""\
declare void @inc()

define void @inc2()
{
  call void @inc()
  call void @inc()
  ret void
}
""")
    assert e.intfn("f1") == 7
    e.voidfn("inc2")
    assert e.intfn("f1") == 9
    e.voidfn("inc")
    assert e.intfn("f1") == 10
    e.voidfn("inc2")
    assert e.intfn("f1") == 12

    with pytest.raises(RuntimeError):
        e.add_module("""\
define void @inc2()
{
  ; FAIL: @inc is not defined
  call void @inc()
  call void @inc()
  ret void
}
""")



# Fortran

def test_program():
    e = FortranEvaluator()
    e.evaluate("""\
program test
implicit none
contains

    subroutine sub1(a, b)
    integer, intent(in) :: a
    integer, intent(out) :: b
    b = a + 1
    end subroutine

end program
""")

def test_subroutine():
    e = FortranEvaluator()
    e.evaluate("""\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 1
end subroutine
""")

def test_fn_declaration():
    e = FortranEvaluator()
    e.evaluate("""\
integer function fn0()
fn0 = 5
end function
""")
    e.evaluate("""\
integer function fn1(a)
integer, intent(in) :: a
fn1 = 5
end function
""")
    e.evaluate("""\
integer function fn2(a, b)
integer, intent(in) :: a, b
fn2 = 5
end function
""")
    e.evaluate("""\
integer function fn3(a, b, c)
integer, intent(in) :: a, b, c
fn3 = 5
end function
""")

def test_fn_call1():
    e = FortranEvaluator()
    e.evaluate("""\
integer function fn()
fn = 5
end function
""")
    assert e.evaluate("fn()+5") == 10
    assert e.evaluate("fn()+6") == 11

def test_fn_call2():
    e = FortranEvaluator()
    e.evaluate("""\
integer function fn(a, b)
integer, intent(in) :: a, b
fn = a + b
end function
""")
    # TODO: the arguments must be passed as pointers:
    #assert e.evaluate("fn(2, 3)") == 5
    #assert e.evaluate("fn(5, -3)") == 2

def test_simple_arithmetics():
    e = FortranEvaluator()
    assert e.evaluate("""\
5+5
""") == 10
    assert e.evaluate("""\
5+6
""") == 11

def test_variables():
    e = FortranEvaluator()
    assert "a" not in e.symbol_table
    e.evaluate("""\
integer :: a
""")
    assert "a" in e.symbol_table
    e.evaluate("""\
a = 5
""")
    assert "a" in e.symbol_table
    assert e.evaluate("""\
a
""") == 5
    assert "a" in e.symbol_table