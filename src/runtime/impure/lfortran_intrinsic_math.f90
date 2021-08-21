module lfortran_intrinsic_math
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
use, intrinsic :: iso_c_binding, only: c_float, c_double
implicit none

interface abs
    module procedure sabs, dabs, cabs, zabs
end interface

interface sqrt
    module procedure ssqrt, dsqrt
end interface

interface exp
    module procedure sexp, dexp
end interface

interface log
    module procedure slog, dlog
end interface

interface erf
    module procedure serf, derf
end interface

interface sin
    module procedure ssin, dsin, csin, zsin
end interface

interface cos
    module procedure scos, dcos, ccos, zcos
end interface

interface tan
    module procedure stan, dtan, ctan, ztan
end interface

interface sinh
    module procedure ssinh, dsinh, csinh, zsinh
end interface

interface cosh
    module procedure scosh, dcosh, ccosh, zcosh
end interface

interface tanh
    module procedure stanh, dtanh, ctanh, ztanh
end interface

interface asin
    module procedure sasin, dasin, casin, zasin
end interface

interface acos
    module procedure sacos, dacos, cacos, zacos
end interface

interface atan
    module procedure satan, datan, catan, zatan
end interface

interface asinh
    module procedure sasinh, dasinh, casinh, zasinh
end interface

interface acosh
    module procedure sacosh, dacosh, cacosh, zacosh
end interface

interface atanh
    module procedure satanh, datanh, catanh, zatanh
end interface

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

elemental real(sp) function cabs(x) result(r)
complex(sp), intent(in) :: x
r = sqrt(real(x,sp)**2 + aimag(x)**2)
end function

elemental real(dp) function zabs(x) result(r)
complex(dp), intent(in) :: x
r = sqrt(real(x,dp)**2 + aimag(x)**2)
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

! exp --------------------------------------------------------------------------

elemental real(sp) function sexp(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_sexp(x) bind(c, name="_lfortran_sexp")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_sexp(x)
end function

elemental real(dp) function dexp(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dexp(x) bind(c, name="_lfortran_dexp")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dexp(x)
end function

! log --------------------------------------------------------------------------

elemental real(sp) function slog(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_slog(x) bind(c, name="_lfortran_slog")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_slog(x)
end function

elemental real(dp) function dlog(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dlog(x) bind(c, name="_lfortran_dlog")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dlog(x)
end function

! erf --------------------------------------------------------------------------

elemental real(sp) function serf(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_serf(x) bind(c, name="_lfortran_serf")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_serf(x)
end function

elemental real(dp) function derf(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_derf(x) bind(c, name="_lfortran_derf")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_derf(x)
end function

! sin --------------------------------------------------------------------------

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

! cos --------------------------------------------------------------------------

elemental real(sp) function scos(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_scos(x) bind(c, name="_lfortran_scos")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_scos(x)
end function

elemental real(dp) function dcos(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dcos(x) bind(c, name="_lfortran_dcos")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dcos(x)
end function

elemental complex(sp) function ccos(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_ccos(x) bind(c, name="_lfortran_ccos")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_ccos(x)
end function

elemental complex(dp) function zcos(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zcos(x) bind(c, name="_lfortran_zcos")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zcos(x)
end function

! tan --------------------------------------------------------------------------

elemental real(sp) function stan(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_stan(x) bind(c, name="_lfortran_stan")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_stan(x)
end function

elemental real(dp) function dtan(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dtan(x) bind(c, name="_lfortran_dtan")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dtan(x)
end function

elemental complex(sp) function ctan(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_ctan(x) bind(c, name="_lfortran_ctan")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_ctan(x)
end function

elemental complex(dp) function ztan(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_ztan(x) bind(c, name="_lfortran_ztan")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_ztan(x)
end function

! sinh --------------------------------------------------------------------------

elemental real(sp) function ssinh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_ssinh(x) bind(c, name="_lfortran_ssinh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_ssinh(x)
end function

elemental real(dp) function dsinh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dsinh(x) bind(c, name="_lfortran_dsinh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dsinh(x)
end function

elemental complex(sp) function csinh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_csinh(x) bind(c, name="_lfortran_csinh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_csinh(x)
end function

elemental complex(dp) function zsinh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zsinh(x) bind(c, name="_lfortran_zsinh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zsinh(x)
end function

! cosh --------------------------------------------------------------------------

elemental real(sp) function scosh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_scosh(x) bind(c, name="_lfortran_scosh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_scosh(x)
end function

elemental real(dp) function dcosh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dcosh(x) bind(c, name="_lfortran_dcosh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dcosh(x)
end function

elemental complex(sp) function ccosh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_ccosh(x) bind(c, name="_lfortran_ccosh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_ccosh(x)
end function

elemental complex(dp) function zcosh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zcosh(x) bind(c, name="_lfortran_zcosh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zcosh(x)
end function

! tanh --------------------------------------------------------------------------

elemental real(sp) function stanh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_stanh(x) bind(c, name="_lfortran_stanh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_stanh(x)
end function

elemental real(dp) function dtanh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dtanh(x) bind(c, name="_lfortran_dtanh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dtanh(x)
end function

elemental complex(sp) function ctanh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_ctanh(x) bind(c, name="_lfortran_ctanh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_ctanh(x)
end function

elemental complex(dp) function ztanh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_ztanh(x) bind(c, name="_lfortran_ztanh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_ztanh(x)
end function

! asin --------------------------------------------------------------------------

elemental real(sp) function sasin(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_sasin(x) bind(c, name="_lfortran_sasin")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_sasin(x)
end function

elemental real(dp) function dasin(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dasin(x) bind(c, name="_lfortran_dasin")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dasin(x)
end function

elemental complex(sp) function casin(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_casin(x) bind(c, name="_lfortran_casin")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_casin(x)
end function

elemental complex(dp) function zasin(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zasin(x) bind(c, name="_lfortran_zasin")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zasin(x)
end function

! acos --------------------------------------------------------------------------

elemental real(sp) function sacos(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_sacos(x) bind(c, name="_lfortran_sacos")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_sacos(x)
end function

elemental real(dp) function dacos(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dacos(x) bind(c, name="_lfortran_dacos")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dacos(x)
end function

elemental complex(sp) function cacos(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_cacos(x) bind(c, name="_lfortran_cacos")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_cacos(x)
end function

elemental complex(dp) function zacos(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zacos(x) bind(c, name="_lfortran_zacos")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zacos(x)
end function

! atan --------------------------------------------------------------------------

elemental real(sp) function satan(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_satan(x) bind(c, name="_lfortran_satan")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_satan(x)
end function

elemental real(dp) function datan(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_datan(x) bind(c, name="_lfortran_datan")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_datan(x)
end function

elemental complex(sp) function catan(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_catan(x) bind(c, name="_lfortran_catan")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_catan(x)
end function

elemental complex(dp) function zatan(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zatan(x) bind(c, name="_lfortran_zatan")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zatan(x)
end function

! asinh --------------------------------------------------------------------------

elemental real(sp) function sasinh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_sasinh(x) bind(c, name="_lfortran_sasinh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_sasinh(x)
end function

elemental real(dp) function dasinh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dasinh(x) bind(c, name="_lfortran_dasinh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dasinh(x)
end function

elemental complex(sp) function casinh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_casinh(x) bind(c, name="_lfortran_casinh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_casinh(x)
end function

elemental complex(dp) function zasinh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zasinh(x) bind(c, name="_lfortran_zasinh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zasinh(x)
end function

! acosh --------------------------------------------------------------------------

elemental real(sp) function sacosh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_sacosh(x) bind(c, name="_lfortran_sacosh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_sacosh(x)
end function

elemental real(dp) function dacosh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_dacosh(x) bind(c, name="_lfortran_dacosh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_dacosh(x)
end function

elemental complex(sp) function cacosh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_cacosh(x) bind(c, name="_lfortran_cacosh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_cacosh(x)
end function

elemental complex(dp) function zacosh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zacosh(x) bind(c, name="_lfortran_zacosh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zacosh(x)
end function

! atanh --------------------------------------------------------------------------

elemental real(sp) function satanh(x) result(r)
real(sp), intent(in) :: x
interface
    pure real(c_float) function c_satanh(x) bind(c, name="_lfortran_satanh")
    import :: c_float
    real(c_float), intent(in), value :: x
    end function
end interface
r = c_satanh(x)
end function

elemental real(dp) function datanh(x) result(r)
real(dp), intent(in) :: x
interface
    pure real(c_double) function c_datanh(x) bind(c, name="_lfortran_datanh")
    import :: c_double
    real(c_double), intent(in), value :: x
    end function
end interface
r = c_datanh(x)
end function

elemental complex(sp) function catanh(x) result(r)
complex(sp), intent(in) :: x
interface
    pure complex(c_float) function c_catanh(x) bind(c, name="_lfortran_catanh")
    import :: c_float
    complex(c_float), intent(in), value :: x
    end function
end interface
r = c_catanh(x)
end function

elemental complex(dp) function zatanh(x) result(r)
complex(dp), intent(in) :: x
interface
    pure complex(c_double) function c_zatanh(x) bind(c, name="_lfortran_zatanh")
    import :: c_double
    complex(c_double), intent(in), value :: x
    end function
end interface
r = c_zatanh(x)
end function

end module
