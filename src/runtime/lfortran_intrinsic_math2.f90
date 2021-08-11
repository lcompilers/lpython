! Temporary module, a subset of lfortran_intrinsic_math that works
module lfortran_intrinsic_math2
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
implicit none

interface abs
    module procedure sabs, dabs
end interface

!interface sqrt
!    module procedure ssqrt, dsqrt
!end interface


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

!elemental real(sp) function ssqrt(x) result(r)
!real(sp), intent(in) :: x
!if (x >= 0) then
!    r = x**(1._sp/2)
!else
!    error stop "sqrt(x) for x < 0 is not allowed"
!end if
!end function

!elemental real(dp) function dsqrt(x) result(r)
!real(dp), intent(in) :: x
!if (x >= 0) then
!    r = x**(1._dp/2)
!else
!    error stop "sqrt(x) for x < 0 is not allowed"
!end if
!end function


end module
