program array_op
implicit none

integer :: a(4), b(4), c(4)
integer :: i, j

i = 0

a = [i + 1, (i + j, j = 1, 2), i + 4]

b = [4*a(i + 1), 5*a(i + 2), 6*a(i + 3), 7*a(i + 4)]

! c = a + b
! print *, c

! c = a - b
! print *, c

! c = a*b
! print *, c

! c = b/a
! print *, c

end program