program doloop_02
implicit none
integer :: i, j, a, b
j = 0
a = 1
b = 10
do i = a, b
    j = j + i
end do
if (j /= 55) error stop

a = 0
do i = 1, 10
    do j = 1, 10
        a = a + (i-1)*10+j
    end do
end do
if (a /= 100*101/2) error stop

a = 0
do i = 1, 10
    do j = 1, i
        a = a + j
    end do
end do
if (a /= 220) error stop
end
