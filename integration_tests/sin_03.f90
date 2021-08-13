program sin_03
use iso_fortran_env, only: dp => real64
implicit none
real(dp) :: x
x = sin(1.5_dp)
print *, x
end program
