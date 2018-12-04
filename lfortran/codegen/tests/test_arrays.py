from lfortran.codegen.evaluator import FortranEvaluator

def test_arrays1():
    e = FortranEvaluator()
    e.evaluate("""\
integer :: i, a(3)
do i = 1, 3
    a(i) = i+10
end do
""")
    assert e.evaluate("a(1)") == 11
    assert e.evaluate("a(2)") == 12
    assert e.evaluate("a(3)") == 13

    e.evaluate("""\
integer :: b(4)
do i = 11, 14
    b(i-10) = i
end do
""")
    assert e.evaluate("b(1)") == 11
    assert e.evaluate("b(2)") == 12
    assert e.evaluate("b(3)") == 13
    assert e.evaluate("b(4)") == 14

    e.evaluate("""\
do i = 1, 3
    b(i) = a(i)-10
end do
""")
    assert e.evaluate("b(1)") == 1
    assert e.evaluate("b(2)") == 2
    assert e.evaluate("b(3)") == 3

    e.evaluate("b(4) = b(1)+b(2)+b(3)+a(1)")
    assert e.evaluate("b(4)") == 17

    e.evaluate("b(4) = a(1)")
    assert e.evaluate("b(4)") == 11

def test_arrays2():
    e = FortranEvaluator()
    e.evaluate("""\
integer, parameter :: dp = kind(0.d0)
real(dp) :: a(3), b, c
integer :: i
a(1) = 3._dp
a(2) = 2._dp
a(3) = 1._dp
b = sum(a)
if (abs(b-6._dp) < 1e-12_dp) then
    i = 1
else
    i = 2
end if
""")
    assert e.evaluate("i") == 1