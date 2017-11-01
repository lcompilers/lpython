program expr_02
implicit none
integer :: x
x = (2+3)*5
if (x /= 25) stop 1

x = (2+3)*4
if (x /= 20) stop 1

x = (2+3)*(2+3)
if (x /= 25) stop 1

x = (2+3)*(2+3)*4*2*(1+2)
if (x /= 600) stop 1
end program
