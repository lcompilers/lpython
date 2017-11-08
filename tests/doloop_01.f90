program doloop_01
implicit none
integer :: i, j
j = 0
do i = 1, 10
    j = j + i
end do
if (j /= 55) error stop

j = 0
do i = 10, 1, -1
    j = j + i
end do
if (j /= 55) error stop

j = 0
do i = 1, 9, 2
    j = j + i
end do
if (j /= 25) error stop

j = 0
do i = 1, 10, 2
    j = j + i
end do
if (j /= 25) error stop

j = 0
do i = 1, 10, 3
    j = j + i
end do
if (j /= 22) error stop

j = 0
do i = 1, 1
    j = j + i
end do
if (j /= 1) error stop

j = 0
do i = 1, 0
    j = j + i
end do
if (j /= 0) error stop
end
