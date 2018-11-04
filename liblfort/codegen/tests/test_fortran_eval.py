import pytest

from liblfort.codegen.evaluator import FortranEvaluator, LLVMEvaluator

# Tests for FortranEvaluator

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

def test_f_call0():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f()
f = 5
end function
""")
    assert e.evaluate("f()+5") == 10
    assert e.evaluate("f()+6") == 11

def test_f_call1():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a)
integer, intent(in) :: a
f = a + 5
end function
""")
    assert e.evaluate("f(2)") == 7
    assert e.evaluate("f(5)") == 10
    e.evaluate("integer :: i")
    e.evaluate("i = 2")
    assert e.evaluate("f(i)") == 7
    e.evaluate("i = 5")
    assert e.evaluate("f(i)") == 10

def test_f_call2():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a, b)
integer, intent(in) :: a, b
f = a + b
end function
""")
    assert e.evaluate("f(2, 3)") == 5
    assert e.evaluate("f(5, -3)") == 2
    e.evaluate("integer :: i")
    e.evaluate("i = -3")
    assert e.evaluate("f(5, i)") == 2

def test_f_call3():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a, b, c)
integer, intent(in) :: a, b, c
f = a + b + c
end function
""")
    assert e.evaluate("f(2, 3, 4)") == 9
    assert e.evaluate("f(5, -3, -1)") == 1
    e.evaluate("integer :: i")
    e.evaluate("i = -3")
    assert e.evaluate("f(5, i, -1)") == 1

def test_simple_arithmetics():
    e = FortranEvaluator()
    assert e.evaluate("5+5") == 10
    assert e.evaluate("5+6") == 11

def test_whitespace1():
    e = FortranEvaluator()
    e.evaluate("integer :: a")
    e.evaluate("a = 5")
    assert e.evaluate("a") == 5

def test_whitespace2():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: a
""")
    e.evaluate("""\
a = 5
""")
    assert e.evaluate("""\
a
""") == 5

def test_variables1():
    e = FortranEvaluator()
    assert not e._global_scope.resolve("a", False)
    e.evaluate("integer :: a")
    assert e._global_scope.resolve("a", False)
    e.evaluate("a = 5")
    assert e._global_scope.resolve("a", False)
    assert e.evaluate("a") == 5
    assert e._global_scope.resolve("a", False)
    assert e.evaluate("a+3") == 8

def test_variables2():
    e = FortranEvaluator()
    e.evaluate("integer :: a")
    assert e._global_scope.resolve("a", False)
    assert not e._global_scope.resolve("b", False)
    e.evaluate("integer :: b")
    assert e._global_scope.resolve("a", False)
    assert e._global_scope.resolve("b", False)
    e.evaluate("a = 5")
    assert e.evaluate("a") == 5
    assert e._global_scope.resolve("a", False)
    assert e._global_scope.resolve("b", False)
    e.evaluate("b = a")
    assert e.evaluate("a") == 5
    assert e.evaluate("b") == 5
    e.evaluate("b = a + 3")
    assert e.evaluate("a") == 5
    assert e.evaluate("b") == 8
    assert e.evaluate("(a+b)*2") == 26
