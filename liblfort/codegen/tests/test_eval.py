from liblfort.codegen.evaluator import FortranEvaluator

def test_eval1():
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

def test_eval2():
    e = FortranEvaluator()
    e.evaluate("""\
subroutine sub1(a, b)
integer, intent(in) :: a
integer, intent(out) :: b
b = a + 1
end subroutine
""")

def test_eval3():
    e = FortranEvaluator()
    e.evaluate("""\
integer function fn()
fn = 5
end function
""")
#    assert e.evaluate("fn()+5") == 10
#    assert e.evaluate("fn()+6") == 11

def test_eval4():
    e = FortranEvaluator()
    assert e.evaluate("""\
5+5
""") == 10
    assert e.evaluate("""\
5+6
""") == 11