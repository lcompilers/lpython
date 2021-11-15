! Temporary module, a subset of lfortran_intrinsic_math that works
module lfortran_intrinsic_math2
use, intrinsic :: iso_fortran_env, only: i32 => int32, sp => real32, dp => real64
implicit none

interface abs
    module procedure iabs, sabs, dabs, cabs, zabs
end interface

interface sqrt
    module procedure ssqrt, dsqrt
end interface

interface aimag
    module procedure saimag, daimag
end interface

interface floor
    module procedure sfloor, dfloor
end interface

interface nint
    module procedure snint, dnint
end interface

interface modulo
    module procedure imodulo, smodulo, dmodulo
end interface

interface mod
    module procedure imod, smod, dmod
end interface

interface min
    module procedure imin, smin, dmin, imin_6args
end interface

interface max
    module procedure imax, smax, dmax, imax_6args
end interface

interface huge
    module procedure i32huge, sphuge, dphuge
end interface

contains

! abs --------------------------------------------------------------------------

elemental integer function iabs(x) result(r)
integer, intent(in) :: x
if (x >= 0) then
    r = x
else
    r = -x
end if
end function

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

elemental real(sp) function cabs(x) result(r)
complex(sp), intent(in) :: x
r = 1.0
end function

elemental real(dp) function zabs(x) result(r)
complex(dp), intent(in) :: x
r = 1.0
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

! aimag ------------------------------------------------------------------------

elemental real(sp) function saimag(x) result(r)
complex(sp), intent(in) :: x
r = 3
! Uncomment once it is implemented
!r = x%im
!error stop "aimag not implemented yet"
end function

elemental real(dp) function daimag(x) result(r)
complex(dp), intent(in) :: x
r = 3
! Uncomment once it is implemented
!r = x%im
!error stop "aimag not implemented yet"
end function

! floor ------------------------------------------------------------------------

elemental integer function sfloor(x) result(r)
real(sp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = x-1
end if
end function

elemental integer function dfloor(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x
else
    r = x-1
end if
end function

! nint ------------------------------------------------------------------------

elemental integer function snint(x) result(r)
real(sp), intent(in) :: x
if (x >= 0) then
    r = x+0.5_sp
else
    r = x-1+0.5_sp
end if
end function

elemental integer function dnint(x) result(r)
real(dp), intent(in) :: x
if (x >= 0) then
    r = x+0.5_dp
else
    r = x-1+0.5_dp
end if
end function

! modulo -----------------------------------------------------------------------

elemental integer function imodulo(x, y) result(r)
integer, intent(in) :: x, y
r = x-floor(real(x)/y)*y
end function

elemental real(sp) function smodulo(x, y) result(r)
real(sp), intent(in) :: x, y
r = x-floor(x/y)*y
end function

elemental real(dp) function dmodulo(x, y) result(r)
real(dp), intent(in) :: x, y
r = x-floor(x/y)*y
end function

! mod --------------------------------------------------------------------------

elemental integer function imod(x, y) result(r)
integer, intent(in) :: x, y
r = x-floor(real(x)/y)*y
if (x < 0 .and. y < 0) return
if (x < 0) r = r - y
if (y < 0) r = r - y
end function

elemental real(sp) function smod(x, y) result(r)
real(sp), intent(in) :: x, y
r = x-floor(x/y)*y
if (x < 0 .and. y < 0) return
if (x < 0) r = r - y
if (y < 0) r = r - y
end function

elemental real(dp) function dmod(x, y) result(r)
real(dp), intent(in) :: x, y
r = x-floor(x/y)*y
if (x < 0 .and. y < 0) return
if (x < 0) r = r - y
if (y < 0) r = r - y
end function

! min --------------------------------------------------------------------------

elemental integer function imin(x, y) result(r)
integer, intent(in) :: x, y
if (x < y) then
    r = x
else
    r = y
end if
end function

elemental real(sp) function smin(x, y) result(r)
real(sp), intent(in) :: x, y
if (x < y) then
    r = x
else
    r = y
end if
end function

elemental real(dp) function dmin(x, y) result(r)
real(dp), intent(in) :: x, y
if (x < y) then
    r = x
else
    r = y
end if
end function

elemental integer function imin_6args(a, b, c, d, e, f) result(r)
integer, intent(in) :: a, b, c, d, e, f
integer :: args(6)
integer :: itr, curr_value
args(1) = a
args(2) = b
args(3) = c
args(4) = d
args(5) = e
args(6) = f
r = a
do itr = 1, 6
    curr_value = args(itr)
    r = min(curr_value, r)
end do
end function

! max --------------------------------------------------------------------------

elemental integer function imax(x, y) result(r)
integer, intent(in) :: x, y
if (x > y) then
    r = x
else
    r = y
end if
end function

elemental integer function imax_6args(a, b, c, d, e, f) result(r)
integer, intent(in) :: a, b, c, d, e, f
integer :: args(6)
integer :: itr, curr_value
args(1) = a
args(2) = b
args(3) = c
args(4) = d
args(5) = e
args(6) = f
r = a
do itr = 1, 6
    curr_value = args(itr)
    r = imax(curr_value, r)
end do
end function

elemental real(sp) function smax(x, y) result(r)
real(sp), intent(in) :: x, y
if (x > y) then
    r = x
else
    r = y
end if
end function

elemental real(dp) function dmax(x, y) result(r)
real(dp), intent(in) :: x, y
if (x > y) then
    r = x
else
    r = y
end if
end function

! huge -------------------------------------------------------------------------

elemental integer(i32) function i32huge(x) result(r)
integer(i32), intent(in) :: x
r = 2147483647
! r = 2**31 - 1
end function

elemental real(sp) function sphuge(x) result(r)
real(sp), intent(in) :: x
r = 3.40282347e38
! r = 2**128 * (1 - 2**-24)
end function

elemental real(dp) function dphuge(x) result(r)
real(dp), intent(in) :: x
r = 1.7976931348623157d308
! r = 2**1024 * (1 - 2**-53)
end function

end module
