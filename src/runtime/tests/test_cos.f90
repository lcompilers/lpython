program test_cos
use lfortran_intrinsic_math, only: cos
implicit none
real :: x
x = cos(9.5)
if (abs(x - (-0.997172177)) > 1e-5) error stop
end program
