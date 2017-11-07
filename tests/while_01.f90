program while_01
implicit none
integer :: i, j
i = 1
j = 0
do while (i < 11)
    j = j + i
    i = i + 1
end do
if (j /= 55) error stop
if (i /= 11) error stop

i = 1
j = 0
do while (i <= 10)
    j = j + i
    i = i + 1
end do
if (j /= 55) error stop
if (i /= 11) error stop

i = 1
j = 0
do while (i < 1)
    j = j + i
    i = i + 1
end do
if (j /= 0) error stop
if (i /= 1) error stop
end
