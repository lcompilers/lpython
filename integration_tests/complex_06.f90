program complex_06
implicit none

real, parameter :: a = 3.0, b = 4.0
complex, parameter :: i_ = (0, 1)
complex, parameter :: z = a + i_*b
real, parameter :: x = z
real, parameter :: y = real(z)
real, parameter :: w = aimag(z)

print *, x, y, w

end program
