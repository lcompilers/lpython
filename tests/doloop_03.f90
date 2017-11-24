program doloop_03
implicit none
integer :: i, j
j = 0
do i = 1, 10
    j = j + i
    if (i == 2) exit
end do
if (j /= 3) error stop
end
