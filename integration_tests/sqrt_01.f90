program sqrt_01
implicit none
real :: x
x = sqrt(4.0)
if (abs(x - 2) > 1e-5) error stop
x = sqrt(2.0)
if (abs(x - 1.4142135623730951) > 1e-5) error stop
end program
