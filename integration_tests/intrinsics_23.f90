program intrinsics_23
implicit none
integer, parameter :: x = huge(0)
real(kind=4), parameter :: y = huge(0.0)
real(kind=8), parameter :: z = huge(0.0d0)

print *, x, y, z
print *, huge(0), huge(0.0), huge(0.0d0)

end program
