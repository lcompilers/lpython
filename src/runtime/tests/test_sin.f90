program test_sin
use lfortran_intrinsic_math, only: sin
implicit none
real :: x
x = sin(1.5)
if (abs(x - 0.997494996) > 1e-5) error stop
end program
