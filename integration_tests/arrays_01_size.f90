program arrays_01
implicit none
integer :: i, a(3), b(4)
integer :: size_a
integer :: size_b
size_a = size(a)
size_b = size(b)
do i = 1, size_a
    a(i) = i+10
end do
if (a(1) /= 11) error stop
if (a(2) /= 12) error stop
if (a(3) /= 13) error stop

do i = 11, 10 + size_b
    b(i-10) = i
end do
if (b(1) /= 11) error stop
if (b(2) /= 12) error stop
if (b(3) /= 13) error stop
if (b(4) /= 14) error stop

do i = 1, size_a
    b(i) = a(i)-10
end do
if (b(1) /= 1) error stop
if (b(2) /= 2) error stop
if (b(3) /= 3) error stop

b(4) = b(1)+b(2)+b(3)+a(1)
if (b(4) /= 17) error stop

b(4) = a(1)
if (b(4) /= 11) error stop
end
