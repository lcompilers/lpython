program types_06
implicit none
real :: r
integer :: i
r = 2
i = 2

if (i < i) error stop
if (r < r) error stop
if (r < i) error stop
if (i < r) error stop

end program
