program complex_02
implicit none

complex :: x, z, w, i_
real :: a, b

x = (1.0,-3.0)
a = 3.0
b = 4.0
i_ = (0, 1)
z = a + i_*b
w = a+b + i_*(a-b)

print *, x, z, w

end program
