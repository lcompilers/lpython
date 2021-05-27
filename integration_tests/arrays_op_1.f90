program array_op_1
implicit none

integer :: a(4), b(4), c(4)
integer :: i, j

i = 0

a = [i + 1, (i + j, j = 1, i + 3), i + 4]

b = [4*a(i + 1), 5*a(i + 2), 6*a(i + 3), 7*a(i + 4)]

print *, a(1), a(2), a(3), a(4)

print *, b(1), b(2), b(3), b(4)

c = a + b
print *, c(1), c(2), c(3), c(4)

c = a - b
print *, c(1), c(2), c(3), c(4)

c = a*b
print *, c(1), c(2), c(3), c(4)

c = b/a
print *, c(1), c(2), c(3), c(4)

end program