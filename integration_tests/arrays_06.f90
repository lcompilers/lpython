program arrays_06
implicit none
integer, dimension(2, 2) :: A
integer :: i, j
integer, dimension(6) :: x
j = 4
x = [(i*2, i = 1, 6)]
print *, x(1), x(2), x(3), x(4), x(5), x(6)
x = [(i+1, i*2, i = 1, 3)]
print *, x(1), x(2), x(3), x(4), x(5), x(6)
x = [(i+1, i**2, i*2, i = 1, 2)]
print *, x(1), x(2), x(3), x(4), x(5), x(6)
x = [(2*i, 3*i, 4*i, i+1, i**2, i*2, i = 2, 2)]
print *, x(1), x(2), x(3), x(4), x(5), x(6)
A = 3.0 + 4
print *, A(1, 1), A(1, 2), A(2, 1), A(2, 2)
! print '("Matrix A"/(10F8.2))',  ((A(i, j), i = 1, 2), j = 1, 2)
end program
