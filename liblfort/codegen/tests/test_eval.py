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
