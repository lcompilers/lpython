program arrays_01_real
implicit none
integer :: i, j
real(8) :: a(3), b(4), c(2, 2)
do i = 1, 3
    a(i) = i+10
end do
if (a(1) /= 11) error stop
if (a(2) /= 12) error stop
if (a(3) /= 13) error stop

do i = 11, 14
    b(i-10) = i
end do
if (b(1) /= 11) error stop
if (b(2) /= 12) error stop
if (b(3) /= 13) error stop
if (b(4) /= 14) error stop

do i = 1, 3
    b(i) = a(i)-10
end do
if (b(1) /= 1) error stop
if (b(2) /= 2) error stop
if (b(3) /= 3) error stop

b(4) = b(1)+b(2)+b(3)+a(1)
if (b(4) /= 17) error stop

b(4) = a(1)
if (b(4) /= 11) error stop

do i = 1, 2
    do j = 1, 2
        c(i, j) = i + j + 10
    end do
end do
if (c(1, 1) /= 12) error stop
if (c(1, 2) /= 13) error stop
if (c(2, 1) /= 13) error stop
if (c(2, 2) /= 14) error stop
end
