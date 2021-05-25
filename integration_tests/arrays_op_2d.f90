program array_op_2d
implicit none

integer :: a(2, 2), b(2, 2), c(2, 2)
integer :: i, j

do i = 1, 2
    do j = 1, 2
        a(i, j) = i*i
        b(i, j) = j*j
    end do
end do

i = 0

print *, a(1, 1), a(1, 2), a(2, 1), a(2, 2)

print *, b(1, 1), b(1, 2), b(2, 1), b(2, 2)

c = a + b
print *, c(1, 1), c(1, 2), c(2, 1), c(2, 2)

c = a - b
print *, c(1, 1), c(1, 2), c(2, 1), c(2, 2)

c = a*b
print *, c(1, 1), c(1, 2), c(2, 1), c(2, 2)

c = b/a
print *, c(1, 1), c(1, 2), c(2, 1), c(2, 2)

end program