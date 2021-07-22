program allocate_01
implicit none
integer, allocatable :: a(:), b(:), c(:)
integer :: n, ierr
n = 10
allocate(a(n), b(n), c(n), stat=ierr)

a = 1
b = 2
c = 3

print *, whole_square(a, b, c)

contains

integer function whole_square(a, b, c) result(status)
implicit none

integer, allocatable :: a(:), b(:), c(:)
integer, allocatable :: a_2(:), b_2(:), c_2(:)
integer, allocatable :: ab(:), bc(:), ca(:), abc(:)

integer :: n
n = size(a)
allocate(a_2(n), b_2(n), c_2(n))
allocate(ab(n), bc(n), ca(n), abc(n))

status = 0

a_2 = a*a
b_2 = b*b
c_2 = c*c
ab = 2*a*b
bc = 2*b*c
ca = 2*c*a
abc = 2*a*b*c

status = 1

end function whole_square

end
