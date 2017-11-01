program expr_02
implicit none
integer :: x
x = (2+3)*5
if (x /= 25) stop 1
end program
