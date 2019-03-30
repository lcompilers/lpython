from lfortran.codegen.evaluator import FortranEvaluator
from lfortran.tests.utils import linux_only

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

def test_f_call_outarg():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 5
f = 0
end function
""")
    e.evaluate("integer :: i")
    assert e.evaluate("f(2, i)") == 0
    assert e.evaluate("i") == 7

def test_f_call_real_1():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a)
real, intent(in) :: a
f = 0
if (a > 2.7) f = 1
end function
""")
    assert e.evaluate("f(2.8)") == 1
    assert e.evaluate("f(2.6)") == 0

def test_f_call_real_2():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a, b)
real, intent(in) :: a, b
real :: c
c = a + b
f = 0
if (c > 2.7) f = 1
end function
""")
    assert e.evaluate("f(1.8, 1.0)") == 1
    assert e.evaluate("f(1.6, 1.0)") == 0

def test_f_call_real_int_2():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a, b)
real, intent(in) :: a
integer, intent(in) :: b
f = 0
if (a > 1.7) f = 1
f = f + b
end function
""")
    assert e.evaluate("f(1.8, 0)") == 1
    assert e.evaluate("f(1.6, 0)") == 0
    assert e.evaluate("f(1.8, 1)") == 2
    assert e.evaluate("f(1.6, 1)") == 1

def test_if_then_else_1():
    e = FortranEvaluator()
    e.evaluate("""\
integer function f(a)
real, intent(in) :: a
f = 3
if (a > 2.7) then
    f = 1
else
    f = 0
end if
end function
""")
    assert e.evaluate("f(2.8)") == 1
    assert e.evaluate("f(2.6)") == 0

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

def test_whitespace3():
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

def test_whitespace4():
    e = FortranEvaluator()
    e.evaluate("""\

""")
    e.evaluate(" ")
    e.evaluate("")

def test_multiline1():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: a
a = 5""")
    assert e.evaluate("a") == 5
    e.evaluate("""\
a = 6
a = a + 1""")
    assert e.evaluate("a") == 7
    assert e.evaluate("""\
a = 6
a = a + 2
a""") == 8

def test_multiline2():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: b
b = 5
function f2(a)
integer, intent(in) :: a
f2 = a + b
end function
""")
    assert e.evaluate("f2(2)") == 7
    e.evaluate("b = 6")
    assert e.evaluate("f2(2)") == 8

def test_multiline3():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: b

b = 5

function f2(a)
integer, intent(in) :: a
f2 = a + b
end function
""")
    assert e.evaluate("f2(2)") == 7
    e.evaluate("b = 6")
    assert e.evaluate("f2(2)") == 8


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

# FIXME: This fails on Windows, but it should work there
@linux_only
def test_print(capfd):
    import ctypes
    libc = ctypes.CDLL(None)
    c_stdout = ctypes.c_void_p.in_dll(libc, 'stdout')

    e = FortranEvaluator()
    e.evaluate("""\
integer :: x
x = (2+3)*5
print *, x, 1, 3, x, (2+3)*5+x
""")
    libc.fflush(c_stdout) # The C stdout buffer must be flushed out
    out = capfd.readouterr().out
    assert out == "25 1 3 25 50 \n"

    e.evaluate("""\
print *, "Hello world!"
""")
    libc.fflush(c_stdout)
    out = capfd.readouterr().out
    assert out == "Hello world! \n"

def test_case_sensitivity():
    e = FortranEvaluator()
    e.evaluate("""\
Integer FUNCTION f(a)
INTEGER, Intent(In) :: a
f = a + 5
End Function
""")
    assert e.evaluate("f(2)") == 7
    assert e.evaluate("f(5)") == 10
