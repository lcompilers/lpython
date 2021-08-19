program modules_15
use iso_fortran_env, only: sp=>real32, dp=>real64
use modules_15b, only: f_int_float, f_int_double, f_int_float_value, &
        f_int_double_value, f_int_doublearray
implicit none
integer :: i, a, n
real(sp) :: r32
real(dp) :: r64, X(3)

a = 3
r32 = 5
i = f_int_float(a, r32)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
i = f_int_double(a, r64)
print *, i
if (i /= 8) error stop

a = 3
r32 = 5
i = f_int_float_value(a, r32)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
i = f_int_double_value(a, r64)
print *, i
if (i /= 8) error stop


n = 3
X(1) = 1.1_dp
X(2) = 2.2_dp
X(3) = 3.3_dp
r64 = f_int_doublearray(n, X)
print *, r64
if (abs(r64 - 6.6_dp) > 1e-10_dp) error stop

end
