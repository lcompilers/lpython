program intrinsics_17
use iso_fortran_env, only: sp=>real32, dp=>real64
real(sp) :: x
real(dp) :: y

x = exp(1.5_sp)
print *, x
y = exp(1.5_dp)
print *, y

x = log(1.5_sp)
print *, x
y = log(1.5_dp)
print *, y

x = erf(1.5_sp)
print *, x
y = erf(1.5_dp)
print *, y

x = atan2(1.5_sp, 2.5_sp)
print *, x
y = atan2(1.5_dp, 2.5_dp)
print *, y

end
