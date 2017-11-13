program arrays_01
implicit none
integer :: i, a(3)
do i = 1, 3
    a(i) = i+10
end do
if (a(1) /= 11) error stop
if (a(2) /= 12) error stop
if (a(3) /= 13) error stop
end
