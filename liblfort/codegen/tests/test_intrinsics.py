from liblfort.codegen.evaluator import FortranEvaluator

def test_intrinsics():
    e = FortranEvaluator()
    e.evaluate("""\
integer, parameter :: dp = kind(0.d0)
real(dp) :: a, b, c(4)
integer :: i, r
r = 0
a = 1.1_dp
b = 1.2_dp
if (b-a > 0.2_dp) r = 1
if (abs(b-a) > 0.2_dp) r = 1
if (abs(a-b) > 0.2_dp) r = 1

a = 4._dp
if (abs(sqrt(a)-2._dp) > 1e-12_dp) r = 1

a = 4._dp
if (abs(log(a)-1.3862943611198906_dp) > 1e-12_dp) r = 1

c(1) = -1._dp
c(2) = -1._dp
c(3) = -1._dp
c(4) = -1._dp
call random_number(c)
do i = 1, 4
    if (c(i) < 0._dp) r = 1
    if (c(i) > 1._dp) r = 1
end do
""")
    assert e.evaluate("r") == 0