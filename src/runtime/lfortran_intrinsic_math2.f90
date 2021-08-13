! Temporary module, a subset of lfortran_intrinsic_math that works
module lfortran_intrinsic_math2
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
implicit none

interface abs
    module procedure sabs, dabs
end interface

interface sqrt
    module procedure ssqrt, dsqrt
end interface

interface sin
    module procedure a6dsin
end interface

real(dp), parameter :: pi = 3.1415926535897932384626433832795_dp

contains

! abs --------------------------------------------------------------------------

elemental real(sp) function sabs(x) result(r)
real(sp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = -x
end if
end function

elemental real(dp) function dabs(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = -x
end if
end function

! sqrt -------------------------------------------------------------------------

elemental real(sp) function ssqrt(x) result(r)
real(sp), intent(in) :: x
if (x >= 0) then
    r = x**(1._sp/2)
else
    error stop "sqrt(x) for x < 0 is not allowed"
end if
end function

elemental real(dp) function dsqrt(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x**(1._dp/2)
else
    error stop "sqrt(x) for x < 0 is not allowed"
end if
end function

! sin --------------------

real(dp) function abs3(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = -x
end if
end function

elemental integer function a1floor(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = x-1
end if
end function

elemental real(dp) function a2modulo(x, y) result(r)
real(dp), intent(in) :: x, y
r = x-a1floor(x/y)*y
end function

elemental real(dp) function a3min(x, y) result(r)
real(dp), intent(in) :: x, y
if (x < y) then
    r = x
else
    r = y
end if
end function

elemental real(dp) function a4max(x, y) result(r)
real(dp), intent(in) :: x, y
if (x > y) then
    r = x
else
    r = y
end if
end function

elemental real(dp) function a6dsin(x) result(r)
real(dp), intent(in) :: x
real(dp) :: y
integer :: n
y = a2modulo(x, 2*pi)
y = a3min(y, pi - y)
y = a4max(y, -pi - y)
y = a3min(y, pi - y)
r = a5kernel_dsin(y)
end function

! Accurate on [-pi/2,pi/2] to about 1e-16
elemental real(dp) function a5kernel_dsin(x) result(res)
real(dp), intent(in) :: x
real(dp), parameter :: S1 = 0.9999999999999990771_dp
real(dp), parameter :: S2 = -0.16666666666664811048_dp
real(dp), parameter :: S3 = 8.333333333226519387e-3_dp
real(dp), parameter :: S4 = -1.9841269813888534497e-4_dp
real(dp), parameter :: S5 = 2.7557315514280769795e-6_dp
real(dp), parameter :: S6 = -2.5051823583393710429e-8_dp
real(dp), parameter :: S7 = 1.6046585911173017112e-10_dp
real(dp), parameter :: S8 = -7.3572396558796051923e-13_dp
real(dp) :: z
z = x*x
res = x * (S1+z*(S2+z*(S3+z*(S4+z*(S5+z*(S6+z*(S7+z*S8)))))))
end function

end module
