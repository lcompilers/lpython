program test_sin2
use iso_fortran_env, only: dp => real64
use lfortran_intrinsic_sin, only: sin2 => sin
implicit none
real(dp) :: x, s1, s2, err, err_max
integer :: i
x = sin(1.5_dp)
if (abs(x - 0.997494996_dp) > 1e-5_dp) error stop
x = sin(20.0_dp)
if (abs(x - 0.9129452507276277_dp) > 1e-5_dp) error stop
print *, "i  x  err"
err_max = 0
do i = 1, 2000
    x = i*0.01_dp - 10
    s1 = sin2(x)
    s2 = sin(x)
    err = abs(s1-s2)
    if (err > err_max) err_max = err
    print *, i, x, err
end do
print *
print *, "Maximum error:", err_max
if (err_max > 1e-15_dp) error stop "Maximum error too large"

! Large number
x = 2e10_dp+0.5_dp
s1 = sin2(x); s2 = sin(x); err = abs(s1-s2)
print *, x, err

x = 2e33_dp+0.5_dp
s1 = sin2(x); s2 = sin(x); err = abs(s1-s2)
print *, x, err
end program
