program complex_03
implicit none

complex :: x
real :: a, b

x = (1.0,-3.0)
a = x%re
b = x%im
print *, a, b
print *, x%re, x%im
if (abs(x%re - 1) > 1e-5) error stop
if (abs(x%im - (-3)) > 1e-5) error stop

end program
