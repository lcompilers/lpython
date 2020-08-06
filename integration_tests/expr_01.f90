program expr_01
implicit none
integer :: x
x = (2+3)*5
if (x == 25) error stop
end program
