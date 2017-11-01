program variables_01
implicit none
integer :: x, y
x = 2
y = 3
if (x /= 2) stop 1
if (y /= 3) stop 1

x = y
if (x /= 3) stop 1
if (y /= 3) stop 1
end program
