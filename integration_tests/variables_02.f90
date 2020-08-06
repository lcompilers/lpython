program variables_02
implicit none
integer :: x, y
x = 2
y = x + 1
if (y /= 3) error stop

x = 2
y = 3
y = (x+2)*(1-y)+1
if (y /= -7) error stop

x = 2
y = -x*(-x)
if (y /= 4) error stop
end program
