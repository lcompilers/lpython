program abs_02
use iso_fortran_env, only: dp=>real64
implicit none
real(dp) :: x
x = abs(1.5_dp)
if (x < 0) error stop
x = abs(-1.5_dp)
if (x < 0) error stop
end program
