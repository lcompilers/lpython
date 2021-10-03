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
    s12 = floor(3.6)

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
    d12 = floor(3.6_dp)

print *, abs(-0.5), abs(-0.5_dp), s1, d1
print *, exp(0.5), exp(0.5_dp), s2, d2
print *, log(0.5), log(0.5_dp), s3, d3
print *, erf(0.5), erf(0.5_dp), s4, d4
print *, erfc(0.5), erfc(0.5_dp), s5, d5
print *, sqrt(0.5), sqrt(0.5_dp), s6, d6
print *, gamma(0.5), gamma(0.5_dp), s7, d7
print *, atan2(0.5, 0.5), atan2(0.5_dp, 0.5_dp), s8, d8
print *, log_gamma(0.5), log_gamma(0.5_dp), s9, d9
print *, log10(0.5), log10(0.5_dp), s10, d10
print *, nint(3.6), nint(3.6_dp), s11, d11
print *, floor(3.6), floor(3.6_dp), s12, d12

end
