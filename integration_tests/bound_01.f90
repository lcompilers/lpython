program where_01
implicit none
real(4) :: a(10:15, 1:5)

! print *, size(a)
print *, lbound(a, 1, 8)
! print *, ubound(a, 2)

end program
