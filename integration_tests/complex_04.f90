program complex_04
implicit none

complex :: x
real :: a, b

x = (1.0,-3.0)
a = real(x)
b = aimag(x)
print *, a, b
if (abs(a - 1) > 1e-5) error stop
if (abs(b - (-3)) > 1e-5) error stop

end program
