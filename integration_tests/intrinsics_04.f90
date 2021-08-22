program intrinsics_04
use iso_fortran_env, only: dp=>real64
real(dp) :: x
complex(dp) :: z

x = 1.5_dp
x = tan(x)
print *, x
if (abs(x - 14.101419947171721_dp) > 1e-10_dp) error stop

z = (1.5_dp, 3.5_dp)
z = tan(z)
print *, z
if (abs(real(z,dp) - 2.57834890405532317E-004_dp) > 1e-10_dp) error stop
!if (abs(aimag(z) - 1.0018071108086137_dp) > 1e-10_dp) error stop

end
