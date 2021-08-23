program modules_18
use iso_fortran_env, only: sp=>real32
use modules_18b, only: f
implicit none
integer :: i, a
real(sp) :: r32

a = 3
r32 = 5
i = f(a, r32)
print *, i
if (i /= 8) error stop

end
