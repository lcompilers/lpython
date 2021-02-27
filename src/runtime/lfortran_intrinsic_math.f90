module lfortran_intrinsic_math
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
use, intrinsic :: iso_c_binding, only: c_float, c_double
implicit none

interface abs
    module procedure sabs, dabs, cabs, zabs
end interface

interface sin
    module procedure ssin, dsin, csin, zsin
end interface

interface sqrt
    module procedure ssqrt, dsqrt
end interface

contains

!-------------------------------------------------------------------------------

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
r = sqrt(real(x,sp)**2 + aimag(x)**2)
end function

elemental real(dp) function zabs(x) result(r)
complex(dp), intent(in) :: x
r = sqrt(real(x,dp)**2 + aimag(x)**2)
end function

!-------------------------------------------------------------------------------

elemental real(sp) function ssin(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_ssin(x) bind(c, name="_lfortran_ssin")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_ssin(x)
end function

elemental real(dp) function dsin(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dsin(x) bind(c, name="_lfortran_dsin")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dsin(x)
end function

elemental complex(sp) function csin(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_csin(x) bind(c, name="_lfortran_csin")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_csin(x)
end function

elemental complex(dp) function zsin(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zsin(x) bind(c, name="_lfortran_zsin")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zsin(x)
end function

!-------------------------------------------------------------------------------

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

end module
