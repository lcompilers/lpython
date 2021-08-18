program modules_15
use iso_fortran_env, only: sp=>real32, dp=>real64
use modules_15b, only: f1, f2
implicit none

integer :: i, a
real(sp) :: r32
real(dp) :: r64
a = 3
r32 = 5
r64 = 5
i = f1(a, r32)
print *, i
if (i /= 8) error stop

i = f2(a, r64)
print *, i
if (i /= 8) error stop

end
