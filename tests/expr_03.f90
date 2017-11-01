program expr_03
implicit none
integer :: x
x = -2
if (x /= -2) stop 1

x = -2*3
if (x /= -6) stop 1

x = -2*(-3)
if (x /= 6) stop 1

x = 3 - 1
if (x /= 2) stop 1

x = 1 - 3
if (x /= -2) stop 1
if (x /= (-2)) stop 1

x = 1 - (-3)
if (x /= 4) stop 1
if (x /= +4) stop 1
if (x /= (+4)) stop 1
end program
