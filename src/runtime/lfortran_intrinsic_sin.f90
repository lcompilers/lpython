module lfortran_intrinsic_sin
use, intrinsic :: iso_fortran_env, only: sp => real32, dp => real64
implicit none
private
public sin

interface sin
    module procedure dsin
end interface

real(dp), parameter :: pi = 3.1415926535897932384626433832795_dp

contains

! sin --------------------------------------------------------------------------
! Our implementation here is based off of the C files from the Sun compiler
!
! Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
!
! Developed at SunSoft, a Sun Microsystems, Inc. business.
! Permission to use, copy, modify, and distribute this
! software is freely granted, provided that this notice
! is preserved.

! sin(x)
! https://www.netlib.org/fdlibm/s_sin.c
! Return sine function of x.
!
! kernel function:
!	__kernel_sin		... sine function on [-pi/4,pi/4]
!	__kernel_cos		... cose function on [-pi/4,pi/4]
!	__ieee754_rem_pio2	... argument reduction routine
!
! Method.
!      Let S,C and T denote the sin, cos and tan respectively on
!	[-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
!	in [-pi/4 , +pi/4], and let n = k mod 4.
!	We have
!
!          n        sin(x)      cos(x)        tan(x)
!     ----------------------------------------------------------
!          0          S           C             T
!          1          C          -S            -1/T
!          2         -S          -C             T
!          3         -C           S            -1/T
!     ----------------------------------------------------------
!
! Special cases:
!      Let trig be any of sin, cos, or tan.
!      trig(+-INF)  is NaN, with signals;
!      trig(NaN)    is that NaN;
!
! Accuracy:
!	TRIG(x) returns trig(x) nearly rounded
!


real(dp) function dsin(x) result(r)
real(dp), intent(in) :: x
real(dp) :: y(2)
integer :: n
if (abs(x) < pi/4) then
    r = kernel_dsin(x, 0.0_dp, 0)
else
    n = rem_pio2(x, y)
    select case (modulo(n, 4))
        case (0)
            r =  kernel_dsin(y(1), y(2), 1)
        case (1)
            r =  kernel_dcos(y(1), y(2))
        case (2)
            r = -kernel_dsin(y(1), y(2), 1)
        case default
            r = -kernel_dcos(y(1), y(2))
    end select
end if
end function



!  __kernel_sin( x, y, iy)
! http://www.netlib.org/fdlibm/k_sin.c
!  kernel sin function on [-pi/4, pi/4], pi/4 ~ 0.7854
!  Input x is assumed to be bounded by ~pi/4 in magnitude.
!  Input y is the tail of x.
!  Input iy indicates whether y is 0. (if iy=0, y assume to be 0).
!
!  Algorithm
! 	1. Since sin(-x) = -sin(x), we need only to consider positive x.
! 	2. if x < 2^-27 (hx<0x3e400000 0), return x with inexact if x!=0.
! 	3. sin(x) is approximated by a polynomial of degree 13 on
! 	   [0,pi/4]
! 		  	         3            13
! 	   	sin(x) ~ x + S1*x + ... + S6*x
! 	   where
!
!  	|sin(x)         2     4     6     8     10     12  |     -58
!  	|----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
!  	|  x 					           |
!
! 	4. sin(x+y) = sin(x) + sin'(x')*y
! 		    ~ sin(x) + (1-x*x/2)*y
! 	   For better accuracy, let
! 		     3      2      2      2      2
! 		r = x! (S2+x! (S3+x! (S4+x! (S5+x! S6))))
! 	   then                   3    2
! 		sin(x) = x + (S1*x + (x! (r-y/2)+y))

elemental real(dp) function kernel_dsin(x, y, iy) result(res)
real(dp), intent(in) :: x, y
integer, intent(in) :: iy
real(dp), parameter :: half = 5.00000000000000000000e-01_dp
real(dp), parameter :: S1 = -1.66666666666666324348e-01_dp
real(dp), parameter :: S2 =  8.33333333332248946124e-03_dp
real(dp), parameter :: S3 = -1.98412698298579493134e-04_dp
real(dp), parameter :: S4 =  2.75573137070700676789e-06_dp
real(dp), parameter :: S5 = -2.50507602534068634195e-08_dp
real(dp), parameter :: S6 =  1.58969099521155010221e-10_dp
real(dp) :: z, r, v
if (abs(x) < 2.0_dp**(-27)) then
    res = x
    return
end if
z = x*x
v = z*x
r = S2+z*(S3+z*(S4+z*(S5+z*S6)))
if (iy == 0) then
    res = x + v*(S1+z*r)
else
    res = x-((z*(half*y-v*r)-y)-v*S1)
end if
end function

! __kernel_cos( x,  y )
! https://www.netlib.org/fdlibm/k_cos.c
! kernel cos function on [-pi/4, pi/4], pi/4 ~ 0.785398164
! Input x is assumed to be bounded by ~pi/4 in magnitude.
! Input y is the tail of x.
!
! Algorithm
!	1. Since cos(-x) = cos(x), we need only to consider positive x.
!	2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
!	3. cos(x) is approximated by a polynomial of degree 14 on
!	   [0,pi/4]
!		  	                 4            14
!	   	cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
!	   where the remez error is
!
! 	|              2     4     6     8     10    12     14 |     -58
! 	|cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
! 	|    					               |
!
! 	               4     6     8     10    12     14
!	4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
!	       cos(x) = 1 - x*x/2 + r
!	   since cos(x+y) ~ cos(x) - sin(x)*y
!			  ~ cos(x) - x*y,
!	   a correction term is necessary in cos(x) and hence
!		cos(x+y) = 1 - (x*x/2 - (r - x*y))
!	   For better accuracy when x > 0.3, let qx = |x|/4 with
!	   the last 32 bits mask off, and if x > 0.78125, let qx = 0.28125.
!	   Then
!		cos(x+y) = (1-qx) - ((x*x/2-qx) - (r-x*y)).
!	   Note that 1-qx and (x*x/2-qx) is EXACT here, and the
!	   magnitude of the latter is at least a quarter of x*x/2,
!	   thus, reducing the rounding error in the subtraction.


elemental real(dp) function kernel_dcos(x, y) result(res)
real(dp), intent(in) :: x, y
real(dp), parameter :: one=  1.00000000000000000000e+00_dp
real(dp), parameter :: C1 =  4.16666666666666019037e-02_dp
real(dp), parameter :: C2 = -1.38888888888741095749e-03_dp
real(dp), parameter :: C3 =  2.48015872894767294178e-05_dp
real(dp), parameter :: C4 = -2.75573143513906633035e-07_dp
real(dp), parameter :: C5 =  2.08757232129817482790e-09_dp
real(dp), parameter :: C6 = -1.13596475577881948265e-11_dp
real(dp) :: z, r, qx, hz, a
if (abs(x) < 2.0_dp**(-27)) then
    res = one
    return
end if
z = x*x
r  = z*(C1+z*(C2+z*(C3+z*(C4+z*(C5+z*C6)))))
if (abs(x) < 0.3_dp) then
    res = one - (0.5_dp*z - (z*r - x*y))
else
    if (abs(x) > 0.78125_dp) then
        qx = 0.28125_dp
    else
        qx = abs(x)/4
    end if
    hz = 0.5_dp*z-qx
    a  = one-qx
    res = a - (hz - (z*r-x*y))
end if
end function


integer function rem_pio2(x, y) result(n)
! Computes 128bit float approximation of modulo(x, pi/2) returned
! as the sum of two 64bit floating point numbers y(1)+y(2)
! This function roughly implements:
!   y(1) = modulo(x, pi/2)         ! 64bit float
!   y(2) = modulo(x, pi/2) - y(1)  ! The exact tail
!   n = (x-y(1)) / (pi/2)
! The y(1) is the modulo, and y(2) is a tail. When added directly
! as y(1) + y(2) it will be equal to just y(1) in 64bit floats, but
! if used separately in polynomial approximations one can use y(2) to get
! higher accuracy.
real(dp), intent(in) :: x
real(dp), intent(out) :: y(2)

interface
    integer(c_int) function ieee754_rem_pio2(x, y) bind(c)
    use iso_c_binding, only: c_double, c_int
    real(c_double), value, intent(in) :: x
    real(c_double), intent(out) :: y(2)
    end function
end interface

n = ieee754_rem_pio2(x, y)
end function


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
real(dp), parameter :: pi = 3.1415926535897932384626433832795_dp
real(dp), intent(in) :: x
real(dp) :: y
integer :: n
if (abs(x) < pi/2) then
    r = kernel_dsin2(x)
else ! fold into [-pi/2, pi/2]
    y = modulo(x, 2*pi)
    y = min(y, pi - y)
    y = max(y, -pi - y)
    y = min(y, pi - y)
    r = kernel_dsin2(y)
end if
end function

elemental real(dp) function kernel_dsin2(x) result(res)
  real(dp), intent(in) :: x
  real(dp), parameter :: C1 = 2.0612613395817811826e-18_dp
  real(dp), parameter :: C2 = 0.9999999999999990771_dp
  real(dp), parameter :: C3 = -1.1326311799469310948e-16_dp
  real(dp), parameter :: C4 = -0.16666666666664811048_dp
  real(dp), parameter :: C5 = 4.0441204087065883493e-16_dp
  real(dp), parameter :: C6 = 8.333333333226519387e-3_dp
  real(dp), parameter :: C7 = -5.1082355103624979855e-16_dp
  real(dp), parameter :: C8 = -1.9841269813888534497e-4_dp
  real(dp), parameter :: C9 = 3.431131630096384069e-16_dp
  real(dp), parameter :: C10 = 2.7557315514280769795e-6_dp
  real(dp), parameter :: C11 = -1.6713014856642339287e-16_dp
  real(dp), parameter :: C12 = -2.5051823583393710429e-8_dp
  real(dp), parameter :: C13 = 6.6095338377356955055e-17_dp
  real(dp), parameter :: C14 = 1.6046585911173017112e-10_dp
  real(dp), parameter :: C15 = -1.6627129557672300738e-17_dp
  real(dp), parameter :: C16 = -7.3572396558796051923e-13_dp
  real(dp), parameter :: C17 = 1.7462917763807982697e-18
  ! Remez16
  res = C1 + x * (C2 + x * &
       (C3 + x * (C4 + x * &
       (C5 + x * (C6 + x * &
       (C7 + x * (C8 + x * &
       (C9 + x * (C10 + x * &
       (C11 + x * (C12 + x * &
       (C13 + x * (C14 + x * &
       (C15 + x * (C16 + x * C17)))))))))))))))
  ! Chebyshev32
  ! res = 2.1355496357691975886e-20_dp + x * (1.0000000000000000063_dp + x * (-9.004255532791894035e-18_dp + x * (-0.16666666666666708032_dp + x * (3.9332705471670443079e-16_dp + x * (8.333333333341355733e-3_dp + x * (-6.9028665221660691874e-15_dp + x * (-1.9841269848665583425e-4_dp + x * (6.1976058203375757744e-14_dp + x * (2.7557323147213076849e-6_dp + x * (-3.2138655682600360264e-13_dp + x * (-2.5053432626865828714e-8_dp + x * (1.0468987330682077985e-12_dp + x * (1.6360972193468943658e-10_dp + x * (-2.2731120913506430714e-12_dp + x * (-5.5906235858164260444e-12_dp + x * (3.425556604867707744e-12_dp + x * (5.532635689918605909e-12_dp + x * (-3.6764276518941318983e-12_dp + x * (-4.5930955707654957942e-12_dp + x * (2.8488303025191868967e-12_dp + x * (2.7673107200394155003e-12_dp + x * (-1.59803409654352135405e-12_dp + x * (-1.1963665142242100106e-12_dp + x * (6.4277391325110941334e-13_dp + x * (3.6143518894425348369e-13_dp + x * (-1.8071233122115074043e-13_dp + x * (-7.2405073651171420086e-14_dp + x * (3.3717669642454825042e-14_dp + x * (8.63748093603347239e-15_dp + x * (-3.751088768577678467e-15_dp + x * (-4.642806182921299547e-16_dp + x * 1.8832379977836398444e-16)))))))))))))))))))))))))))))))
end function

end module
