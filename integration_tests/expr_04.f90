program expr_04
implicit none
integer :: x
x = 2**4;
if (x /= 16) error stop
x = 2.**4;
if (x /= 16) error stop
x = 2**4.;
if (x /= 16) error stop
x = 2.**4.;
if (x /= 16) error stop
end program
