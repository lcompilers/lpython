program sin_01
use iso_fortran_env, only: dp => real64
implicit none
real(dp) :: x
x = sin(1.5_dp)
if (abs(x - 0.997494996_dp) > 1e-5_dp) error stop
end program
