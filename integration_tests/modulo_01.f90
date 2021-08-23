program modulo_01
real :: x1
integer :: i1
x1 = 3.3
print*, modulo(x1, 3.)
print*, modulo(x1, 2.)
print*, modulo(x1, 1.)
i1 = 4
print*, modulo(i1, 6)
print*, modulo(i1, 3)
print*, modulo(i1, 1)
end program
