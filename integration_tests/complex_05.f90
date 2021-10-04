program complex_05
implicit none

complex, parameter :: x = (1.0,-3.0)
real, parameter :: a = 3.0, b = 4.0
complex, parameter :: y = (a, b)

complex, parameter :: i_ = (0, 1)
complex, parameter :: z = a + i_*b
complex, parameter :: w = a+b + i_*(a-b)

print *, x, y, z, w

end program
