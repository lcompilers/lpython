program test_inverse_trig
use lfortran_intrinsic_math, only: asin, acos, atan, asinh, acosh, atanh
implicit none
real :: x

x = asin(0.84147098)
if (abs(x - 1.0) > 1e-5) error stop

x = acos(0.54030231)
if (abs(x - 1.0) > 1e-5) error stop

x = atan(1.5574077)
if (abs(x - 1.0) > 1e-5) error stop

x = asinh(1.1752012)
if (abs(x - 1.0) > 1e-5) error stop

x = acosh(1.5430806)
if (abs(x - 1.0) > 1e-5) error stop

x = atanh(0.76159416)
if (abs(x - 1.0) > 1e-5) error stop

end program
