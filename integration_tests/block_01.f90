program block_01
implicit none
integer :: a, b, d
a = 1
b = 2
block
    integer :: b, c
    b = 3
    c = 4
    d = 5
end block
if (b /= 2) error stop
if (d /= 5) error stop
end
