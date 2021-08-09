program const_kind_01
integer, parameter :: sp = 4, dp = 8
real(sp), parameter :: r1 = 1.0_sp
real(dp), parameter :: r2 = 1.0_dp
real :: r3
r3 = 1.0_sp
r3 = 1.0_dp
print *, sp, dp, r1, r2, r3
end program
