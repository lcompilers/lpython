program arrays_01
implicit none
integer :: i, a(3)
do i = 1, 3
    a(i) = i
end do
if (a(1) /= 1) error stop
if (a(2) /= 2) error stop
if (a(3) /= 3) error stop
end
