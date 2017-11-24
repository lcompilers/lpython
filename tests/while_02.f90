program while_02
implicit none
integer :: i, j
i = 0
j = 0
do while (i < 10)
    i = i + 1
    j = j + i
end do
if (j /= 55) error stop
if (i /= 10) error stop

i = 0
j = 0
do while (i < 10)
    i = i + 1
    if (i == 2) cycle
    j = j + i
end do
if (j /= 53) error stop
if (i /= 10) error stop
end
