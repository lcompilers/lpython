program intrinsics_20
integer, parameter :: dp = kind(0.d0)
real, parameter :: pi = acos(-1.)
real(dp), parameter :: pi_dp = acos(-1._dp)

real, parameter :: x = cos((3+4)*7.)
integer :: y
integer, parameter :: y2 = 3

print *, pi
print *, pi_dp
print *, x
y = 3
print *, cos((3+4)*7.)
print *, cos((y+4)*7.)
print *, cos((y2+4)*7.)
print *, cos(x+1)**2 + sin(x+1)**2

end
