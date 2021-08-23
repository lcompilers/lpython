program modules_15
use iso_fortran_env, only: sp=>real32, dp=>real64
use modules_15b, only: &
    f_int_float, f_int_double, &
    sub_int_float, sub_int_double, &
    f_int_float_value, f_int_double_value, &
    sub_int_float_value, sub_int_double_value, &
    f_int_intarray, f_int_floatarray, f_int_doublearray, &
    sub_int_intarray, sub_int_floatarray, sub_int_doublearray, &
    f_int_float_complex, f_int_double_complex, &
    sub_int_float_complex, sub_int_double_complex, &
    f_int_float_complex_value, f_int_double_complex_value, &
    sub_int_float_complex_value, sub_int_double_complex_value, &
    f_int_double_value_name, &
    sub_int_double_value_name
implicit none
integer :: i, a, n, I32(3)
real(sp) :: r32, X32(3)
real(dp) :: r64, X64(3)
complex(sp) :: c32
complex(dp) :: c64

a = 3
r32 = 5
i = f_int_float(a, r32)
print *, i
if (i /= 8) error stop

a = 3
r32 = 5
call sub_int_float(a, r32, i)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
i = f_int_double(a, r64)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
call sub_int_double(a, r64, i)
print *, i
if (i /= 8) error stop

a = 3
c32 = (5._sp, 7._sp)
i = f_int_float_complex(a, c32)
print *, i
if (i /= 15) error stop

a = 3
c32 = (5._sp, 7._sp)
call sub_int_float_complex(a, c32, i)
print *, i
if (i /= 15) error stop

a = 3
c32 = (5._sp, 7._sp)
i = f_int_float_complex_value(a, c32)
print *, i
if (i /= 15) error stop

a = 3
c32 = (5._sp, 7._sp)
call sub_int_float_complex_value(a, c32, i)
print *, i
if (i /= 15) error stop

a = 3
c64 = (5._dp, 7._dp)
i = f_int_double_complex(a, c64)
print *, i
if (i /= 15) error stop

a = 3
c64 = (5._dp, 7._dp)
call sub_int_double_complex(a, c64, i)
print *, i
if (i /= 15) error stop

a = 3
c64 = (5._dp, 7._dp)
i = f_int_double_complex_value(a, c64)
print *, i
if (i /= 15) error stop

a = 3
c64 = (5._dp, 7._dp)
call sub_int_double_complex_value(a, c64, i)
print *, i
if (i /= 15) error stop

a = 3
r32 = 5
i = f_int_float_value(a, r32)
print *, i
if (i /= 8) error stop

a = 3
r32 = 5
call sub_int_float_value(a, r32, i)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
i = f_int_double_value(a, r64)
print *, i
if (i /= 8) error stop
i = f_int_double_value_name(a, r64)
print *, i
if (i /= 8) error stop

a = 3
r64 = 5
call sub_int_double_value(a, r64, i)
print *, i
if (i /= 8) error stop
call sub_int_double_value_name(a, r64, i)
print *, i
if (i /= 8) error stop

n = 3
I32(1) = 1
I32(2) = 2
I32(3) = 3
i = f_int_intarray(n, I32)
print *, i
if (i /= 6) error stop

n = 3
I32(1) = 1
I32(2) = 2
I32(3) = 3
call sub_int_intarray(n, I32, i)
print *, i
if (i /= 6) error stop

n = 3
X32(1) = 1.1_dp
X32(2) = 2.2_dp
X32(3) = 3.3_dp
r32 = f_int_floatarray(n, X32)
print *, r32
if (abs(r32 - 6.6_sp) > 1e-5_dp) error stop

n = 3
X32(1) = 1.1_dp
X32(2) = 2.2_dp
X32(3) = 3.3_dp
call sub_int_floatarray(n, X32, r32)
print *, r32
if (abs(r32 - 6.6_sp) > 1e-5_dp) error stop

n = 3
X64(1) = 1.1_dp
X64(2) = 2.2_dp
X64(3) = 3.3_dp
r64 = f_int_doublearray(n, X64)
print *, r64
if (abs(r64 - 6.6_dp) > 1e-10_dp) error stop

n = 3
X64(1) = 1.1_dp
X64(2) = 2.2_dp
X64(3) = 3.3_dp
call sub_int_doublearray(n, X64, r64)
print *, r64
if (abs(r64 - 6.6_dp) > 1e-10_dp) error stop

end
