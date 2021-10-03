program intrinsics_18
! This program tests all trigonometric intrinsics, both in declarations
! and in executable statements. Single and double precision, real only.
integer, parameter :: dp = kind(0.d0)

real, parameter :: &
    s1 = sin(0.5), &
    s2 = cos(0.5), &
    s3 = tan(0.5), &
    s4 = asin(0.5), &
    s5 = acos(0.5), &
    s6 = atan(0.5), &
    s7 = sinh(0.5), &
    s8 = cosh(0.5), &
    s9 = tanh(0.5), &
    s10 = asinh(0.5), &
    s11 = acosh(1.5), &
    s12 = atanh(0.5)

real(dp), parameter :: &
    d1 = sin(0.5_dp), &
    d2 = cos(0.5_dp), &
    d3 = tan(0.5_dp), &
    d4 = asin(0.5_dp), &
    d5 = acos(0.5_dp), &
    d6 = atan(0.5_dp), &
    d7 = sinh(0.5_dp), &
    d8 = cosh(0.5_dp), &
    d9 = tanh(0.5_dp), &
    d10 = asinh(0.5_dp), &
    d11 = acosh(1.5_dp), &
    d12 = atanh(0.5_dp)

real :: x, x2
real(dp) :: y, y2

x = 0.5
y = 0.5_dp
x2 = 1.5
y2 = 1.5_dp

print *, sin(0.5), sin(0.5_dp), s1, d1, sin(x), sin(y)
print *, cos(0.5), cos(0.5_dp), s2, d2, cos(x), cos(y)
print *, tan(0.5), tan(0.5_dp), s3, d3, tan(x), tan(y)

print *, asin(0.5), asin(0.5_dp), s4, d4, asin(x), asin(y)
print *, acos(0.5), acos(0.5_dp), s5, d5, acos(x), acos(y)
print *, atan(0.5), atan(0.5_dp), s6, d6, atan(x), atan(y)

print *, sinh(0.5), sinh(0.5_dp), s7, d7, sinh(x), sinh(y)
print *, cosh(0.5), cosh(0.5_dp), s8, d8, cosh(x), cosh(y)
print *, tanh(0.5), tanh(0.5_dp), s9, d9, tanh(x), tanh(y)

print *, asinh(0.5), asinh(0.5_dp), s10, d10, asinh(x), asinh(y)
print *, acosh(1.5), acosh(1.5_dp), s11, d11, acosh(x2), acosh(y2)
print *, atanh(0.5), atanh(0.5_dp), s12, d12, atanh(x), atanh(y)

end
