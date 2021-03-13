program test_hyperbolics
use lfortran_intrinsic_math, only: sinh, cosh, tanh
implicit none
real :: x

x = sinh(1.0)
if (abs(x - 1.1752012) > 1e-5) error stop

x = cosh(1.0)
if (abs(x - 1.5430806) > 1e-5) error stop

x = tanh(1.0)
if (abs(x - 0.76159416) > 1e-5) error stop
end program
