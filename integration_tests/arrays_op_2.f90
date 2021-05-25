program array_op_2d
implicit none

integer :: a(2, 2, 1), b(2, 2, 1), c(2, 2, 1)
integer :: i, j, k

do i = 1, 2
    do j = 1, 2
        do k = 1, 1
            a(i, j, k) = i*i
            b(i, j, k) = j*j
        end do
    end do
end do

i = 0

print *, a(1, 1, 1), a(1, 2, 1), a(2, 1, 1), a(2, 2, 1)

print *, b(1, 1, 1), b(1, 2, 1), b(2, 1, 1), b(2, 2, 1)

c = a + b
print *, c(1, 1, 1), c(1, 2, 1), c(2, 1, 1), c(2, 2, 1)

c = a - b
print *, c(1, 1, 1), c(1, 2, 1), c(2, 1, 1), c(2, 2, 1)

c = a*b
print *, c(1, 1, 1), c(1, 2, 1), c(2, 1, 1), c(2, 2, 1)

c = b/a
print *, c(1, 1, 1), c(1, 2, 1), c(2, 1, 1), c(2, 2, 1)

end program