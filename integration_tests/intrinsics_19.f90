program intrinsics_19
! Test intrinsics both declarations and executable statements.
! Single and double precision, real only.
integer, parameter :: dp = kind(0.d0)

real, parameter :: &
    s1 = abs(-0.5), &
    s2 = exp(0.5), &
    s3 = log(0.5), &
    s4 = erf(0.5), &
    s5 = erfc(0.5), &
    s6 = sqrt(0.5), &
    s7 = gamma(0.5), &
    s8 = atan2(0.5, 0.5), &
    s9 = log_gamma(0.5), &
    s10 = log10(0.5)

integer, parameter :: &
    s11 = nint(3.6), &
    s12 = floor(3.6), &
    s13 = nint(-3.6), &
    s14 = floor(-3.6)

real(dp), parameter :: &
    d1 = abs(-0.5_dp), &
    d2 = exp(0.5_dp), &
    d3 = log(0.5_dp), &
    d4 = erf(0.5_dp), &
    d5 = erfc(0.5_dp), &
    d6 = sqrt(0.5_dp), &
    d7 = gamma(0.5_dp), &
    d8 = atan2(0.5_dp, 0.5_dp), &
    d9 = log_gamma(0.5_dp), &
    d10 = log10(0.5_dp)

integer, parameter :: &
    d11 = nint(3.6_dp), &
    d12 = floor(3.6_dp), &
    d13 = nint(-3.6_dp), &
    d14 = floor(-3.6_dp)

real :: x, x2
real(dp) :: y, y2

x = 0.5
y = 0.5_dp
x2 = 3.6
y2 = 3.6_dp

print *, abs(-0.5), abs(-0.5_dp), s1, d1, abs(-x), abs(-y)
print *, exp(0.5), exp(0.5_dp), s2, d2, exp(x), exp(y)
print *, log(0.5), log(0.5_dp), s3, d3, log(x), log(y)
print *, erf(0.5), erf(0.5_dp), s4, d4, erf(x), erf(y)
print *, erfc(0.5), erfc(0.5_dp), s5, d5, erfc(x), erfc(y)
print *, sqrt(0.5), sqrt(0.5_dp), s6, d6, sqrt(x), sqrt(y)
print *, gamma(0.5), gamma(0.5_dp), s7, d7, gamma(x), gamma(y)
print *, atan2(0.5, 0.5), atan2(0.5_dp, 0.5_dp), s8, d8, atan2(x,x), atan2(y,y)
print *, log_gamma(0.5), log_gamma(0.5_dp), s9, d9, log_gamma(x), log_gamma(y)
print *, log10(0.5), log10(0.5_dp), s10, d10, log10(x), log10(y)
print *, nint(3.6), nint(3.6_dp), s11, d11, nint(x2), nint(y2)
print *, floor(3.6), floor(3.6_dp), s12, d12, floor(x2), floor(y2)
print *, nint(-3.6), nint(-3.6_dp), s13, d13, nint(-x2), nint(-y2)
print *, floor(-3.6), floor(-3.6_dp), s14, d14, floor(-x2), floor(-y2)

end
