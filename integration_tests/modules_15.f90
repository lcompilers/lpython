program modules_15
use iso_fortran_env, only: dp=>real64
use modules_15b, only: f
implicit none

integer :: i, a
real(dp) :: b
a = 3
b = 5
i = f(a, b)
print *, i
if (i /= 8) error stop

end
