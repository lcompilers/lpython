program array_op
implicit none

integer :: a(3), b(3), c(3)

a = [1, 2, 3]

b = [4, 10, 18]

c = a + b
print *, c

c = a - b
print *, c

c = a*b
print *, c

c = b/a
print *, c

end program