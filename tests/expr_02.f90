program expr_02
implicit none
integer :: x
x = (2+3)*5
if (x /= 25) error stop

x = (2+3)*4
if (x /= 20) error stop

x = (2+3)*(2+3)
if (x /= 25) error stop

x = (2+3)*(2+3)*4*2*(1+2)
if (x /= 600) error stop

x = x / 60
if (x /= 10) error stop

x = x + 1
if (x /= 11) error stop

x = x - 1
if (x /= 10) error stop
end program
