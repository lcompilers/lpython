program test_sin2
use iso_fortran_env, only: dp => real64
use lfortran_intrinsic_sin, only: sin
implicit none
real(dp) :: x
x = sin(1.5_dp)
if (abs(x - 0.997494996_dp) > 1e-5_dp) error stop
x = sin(20.0_dp)
if (abs(x - 0.9129452507276277_dp) > 1e-5_dp) error stop
end program
