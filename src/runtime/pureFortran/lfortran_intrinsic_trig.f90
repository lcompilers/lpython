module lfortran_intrinsic_trig
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
implicit none
private
public sin, cos

real(dp), parameter :: pi = 3.1415926535897932384626433832795_dp

interface sin
    module procedure dsin2
end interface

interface cos
    module procedure kernel_dcos2
end interface


contains

! Our implementation here is designed around range reduction to [-pi/2, pi/2]
! Subsequently, we fit a 64 bit precision polynomials via Sollya (https://www.sollya.org/)
! -- Chebyshev (32 terms) --
! This has a theoretical approximation error bound of [-7.9489615048122632526e-41;7.9489615048122632526e-41]
! Due to rounding errors; we obtain a maximum error (w.r.t. gfortran) of ~ E-15 over [-10, 10]
! -- Remez (16 terms) -- [DEFAULT] (fewer terms)
! Due to rounding errors; we obtain a maximum error (w.r.t. gfortran) of ~ E-16 over [-10, 10]
! For large values, e.g. 2E10 we have an absolute error of E-7
! For huge(0) we have an absolute error of E-008
! TODO: Deal with very large numbers; the errors get worse above 2E10
! For huge(0.0) we have 3.4028234663852886E+038 -0.52187652333365853       0.99999251142364332        1.5218690347573018
!                          value                    gfortran sin             lfortran sin              absolute error

elemental real(dp) function dsin2(x) result(r)
real(dp), intent(in) :: x
real(dp) :: y
integer :: n
y = modulo(x, 2*pi)
y = min(y, pi - y)
y = max(y, -pi - y)
y = min(y, pi - y)
r = kernel_dsin2(y)
end function

! Accurate on [-pi/2,pi/2] to about 1e-16
elemental real(dp) function kernel_dsin2(x) result(res)
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

elemental real(dp) function kernel_dcos2(x) result(res)
real(dp), intent(in) :: x
real(dp), parameter :: C1 = 0.99999999999999999317_dp
real(dp), parameter :: C2 = 4.3522024034217346524e-18_dp
real(dp), parameter :: C3 = -0.49999999999999958516_dp
real(dp), parameter :: C4 = -8.242872826356848038e-17_dp
real(dp), parameter :: C5 = 4.166666666666261697e-2_dp
real(dp), parameter :: C6 = 4.0485005435941782636e-16_dp
real(dp), parameter :: C7 = -1.3888888888731381616e-3_dp
real(dp), parameter :: C8 = -8.721570096570797013e-16_dp
real(dp), parameter :: C9 = 2.4801587270604889267e-5_dp
real(dp), parameter :: C10 = 9.352687193379247843e-16_dp
real(dp), parameter :: C11 = -2.7557315787234544468e-7_dp
real(dp), parameter :: C12 = -5.2320806585871644286e-16_dp
real(dp), parameter :: C13 = 2.0876532326120694722e-9_dp
real(dp), parameter :: C14 = 1.4637857803935104813e-16_dp
real(dp), parameter :: C15 = -1.146215379106821115e-11_dp
real(dp), parameter :: C16 = -1.6185683697669940221e-17_dp
real(dp), parameter :: C17 = 4.6012969591571265687e-14_dp
! Remez16
res = C1  + x * (C2  + x * &
     (C3  + x * (C4  + x * &
     (C5  + x * (C6  + x * &
     (C7  + x * (C8  + x * &
     (C9  + x * (C10 + x * &
     (C11 + x * (C12 + x * &
     (C13 + x * (C14 + x * &
     (C15 + x * (C16 + x * C17)))))))))))))))
end function

real(dp) function dsin3(x) result(r)
real(dp), intent(in) :: x
real(dp) :: y
integer :: n
if (abs(x) < pi/4) then
    r = kernel_dsin2(x)
else
    n = rem_pio2(x, y)
    select case (modulo(n, 4))
        case (0)
            r =  kernel_dsin2(y)
        case (1)
            r =  kernel_dcos2(y)
        case (2)
            r = -kernel_dsin2(y)
        case default
            r = -kernel_dcos2(y)
    end select
end if
end function

integer function rem_pio2(x, y) result(n)
real(dp), intent(in) :: x
real(dp), intent(out) :: y
y = modulo(x, pi/2)
if (y > pi/4) y = y-pi/2
n = (x-y) / (pi/2)
end function

end module
