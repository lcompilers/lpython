module lfortran_intrinsic_sin
! Implicit dependencies
! abs impure/math
! modulo pure/math2
! min builtin/array
! max builtin/array
use iso_c_binding, only: c_double, c_int
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
    n = rem_pio2_c(x, y)
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


integer function rem_pio2_c(x, y) result(n)
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
