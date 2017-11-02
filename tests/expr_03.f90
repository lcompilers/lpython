program expr_03
implicit none
integer :: x
x = -2
if (x /= -2) error stop

x = -2*3
if (x /= -6) error stop

x = -2*(-3)
if (x /= 6) error stop

x = 3 - 1
if (x /= 2) error stop

x = 1 - 3
if (x /= -2) error stop
if (x /= (-2)) error stop

x = 1 - (-3)
if (x /= 4) error stop
if (x /= +4) error stop
if (x /= (+4)) error stop
end program
