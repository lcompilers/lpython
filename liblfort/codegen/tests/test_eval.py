from liblfort.codegen.evaluator import FortranEvaluator, LLVMEvaluator

# LLVM

def test_llvm_eval1():
    e = LLVMEvaluator()
    assert e.evaluate("""\
; ModuleID = ""
target triple = "unknown-unknown-unknown"
target datalayout = ""

define i64 @f1()
{
.entry:
  %0 = alloca i64, align 4
  store i64 4, i64* %0, align 4
  %1 = load i64, i64* %0, align 4
  ret i64 %1
}
""", intfn="f1") == 4





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