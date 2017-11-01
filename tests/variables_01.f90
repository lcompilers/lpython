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

y = 1
if (y == 1) x = 1
if (y /= 1) x = 2
if (x /= 1) stop 1

y = 2
if (y == 1) x = 1
if (y /= 1) x = 2
if (x /= 2) stop 1

x = 2
y = 1
if (y == 1) x = x + 1
if (y /= 1) x = x - 1
if (x /= 3) stop 1

x = 2
y = 2
if (y == 1) x = x + 1
if (y /= 1) x = x - 1
if (x /= 1) stop 1
end program
