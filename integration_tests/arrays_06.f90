program arrays_06
implicit none
integer, dimension(3, 3, 3) :: A
integer :: i, j, k
integer, dimension(6) :: x
j = 4
x = [(i*2, i = 1, 6)]
print *, x
x = [(i+1, i*2, i = 1, 3)]
print *, x
x = [(i+1, i**2, i*2, i = 1, 2)]
print *, x
x = [(2*i, 3*i, 4*i, i+1, i**2, i*2, i = 2, 2)]
print *, x
A = 3.0 + 4
print *, A
! print '("Matrix A"/(10F8.2))',  ((A(i, j), i = 1, 2), j = 1, 2)
end program
