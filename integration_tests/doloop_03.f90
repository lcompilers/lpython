program doloop_03
implicit none
integer :: i, j
j = 0
do i = 1, 10
    j = j + i
    if (i == 3) then
        continue
    end if
    if (i == 2) exit
end do
if (j /= 3) error stop

j = 0
do i = 1, 10
    if (i == 2) exit
    j = j + i
end do
if (j /= 1) error stop

j = 0
do i = 1, 10
    if (i == 2) cycle
    j = j + i
end do
if (j /= 53) error stop
end
