program where_01
implicit none
real :: a(10:15, 1:5)

print *, kind(a)
print *, lbound(a, 1)
print *, ubound(a, 2)

end program
