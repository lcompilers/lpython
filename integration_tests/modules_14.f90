program modules_14
use modules_14_a, only: a
implicit none

integer :: i
real :: r

i = 5
call a(i)
if (i /= 6) error stop

r = 6
call a(r)
if (r /= 7) error stop

i = 7
call a(i)
if (i /= 8) error stop

end program
