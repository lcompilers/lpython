from glob import glob
import os

from lfortran.ast import src_to_ast

def test_files():
    sources = [file1, file2, file3, file4, file5, file6, file7, file8, file9]
    assert len(sources) == 9
    print("Testing filenames:")
    for source in sources:
        src_to_ast(source)

file1 = """\
module bsplines

use types, only: dp
use lapack, only: dgesv, dgbsv
use utils, only: stop_error
implicit none
private
public bspline, bspline_der, bspline_der2

contains

pure recursive function bspline(t, i, k, r) result(B)
! Returns values of a B-spline.
! There are set of `n` B-splines of order `k`. The mesh is composed of `n+k`
! knots, stored in the t(:) array.
real(dp), intent(in) :: t(:) ! {t_i}, i = 1, 2, ..., n+k
integer, intent(in) :: i ! i = 1..n ?
integer, intent(in) :: k ! Order of the spline k = 1, 2, 3, ...
real(dp), intent(in) :: r(:) ! points to evaluate the spline at
real(dp) :: B(size(r))
if (k == 1) then
    if ((t(size(t)) - t(i+1)) < tiny(1._dp) .and. &
        (t(i+1) - t(i)) > tiny(1._dp)) then
        ! If this is the last non-zero knot span, include the right end point,
        ! to ensure that the last basis function goes to 1.
        where (t(i) <= r .and. r <= t(i+1))
            B = 1
        else where
            B = 0
        end where
    else
        ! Otherwise exclude the right end point
        where (t(i) <= r .and. r < t(i+1))
            B = 1
        else where
            B = 0
        end where
    end if
else
    B = 0
    if (t(i+k-1)-t(i) > tiny(1._dp)) then
        B = B + (r-t(i)) / (t(i+k-1)-t(i)) * bspline(t, i, k-1, r)
    end if
    if (t(i+k)-t(i+1) > tiny(1._dp)) then
        B = B + (t(i+k)-r) / (t(i+k)-t(i+1)) * bspline(t, i+1, k-1, r)
    end if
end if
end function

pure function bspline_der(t, i, k, r) result(B)
! Returns values of a derivative of a B-spline.
! There are set of `n` B-splines of order `k`. The mesh is composed of `n+k`
! knots, stored in the t(:) array.
real(dp), intent(in) :: t(:) ! {t_i}, i = 1, 2, ..., n+k
integer, intent(in) :: i ! i = 1..n ?
integer, intent(in) :: k ! Order of the spline k = 1, 2, 3, ...
real(dp), intent(in) :: r(:) ! points to evaluate the spline at
real(dp) :: B(size(r))
if (k == 1) then
    B = 0
else
    B = 0
    if (t(i+k-1)-t(i) > tiny(1._dp)) then
        B = B + (k-1) / (t(i+k-1)-t(i)) * bspline(t, i, k-1, r)
    end if
    if (t(i+k)-t(i+1) > tiny(1._dp)) then
        B = B - (k-1) / (t(i+k)-t(i+1)) * bspline(t, i+1, k-1, r)
    end if
end if
end function

pure function bspline_der2(t, i, k, r) result(B)
! Returns values of a second derivative of a B-spline.
! There are set of `n` B-splines of order `k`. The mesh is composed of `n+k`
! knots, stored in the t(:) array.
real(dp), intent(in) :: t(:) ! {t_i}, i = 1, 2, ..., n+k
integer, intent(in) :: i ! i = 1..n ?
integer, intent(in) :: k ! Order of the spline k = 1, 2, 3, ...
real(dp), intent(in) :: r(:) ! points to evaluate the spline at
real(dp) :: B(size(r))
if (k == 1) then
    B = 0
else
    B = 0
    if (t(i+k-1)-t(i) > tiny(1._dp)) then
        B = B + (k-1) / (t(i+k-1)-t(i)) * bspline_der(t, i, k-1, r)
    end if
    if (t(i+k)-t(i+1) > tiny(1._dp)) then
        B = B - (k-1) / (t(i+k)-t(i+1)) * bspline_der(t, i+1, k-1, r)
    end if
end if
end function

end module
"""

file2 = """\
! Some constants

module constants
use types, only: dp
implicit none
private
public pi, e_, i_, bohr2ang, ang2bohr, Ha2eV, kB, K2au, density2gcm3, u2au, &
    s2au, kJmol2Ha

! Constants contain more digits than double precision, so that
! they are rounded correctly. Single letter constants contain underscore so
! that they do not clash with user variables ("e" and "i" are frequently used as
! loop variables)
real(dp), parameter :: pi    = 3.1415926535897932384626433832795_dp
real(dp), parameter :: e_    = 2.7182818284590452353602874713527_dp
complex(dp), parameter :: i_ = (0, 1)

real(dp), parameter :: Na = 6.02214129e23_dp ! Avogadro constant
! Standard uncertainty      0.00000027 (Source: 2010 CODATA)
real(dp), parameter :: Ha2eV = 27.21138505_dp    ! 1 Ha = (1 * Ha2eV) eV
! Standard uncertainty is       0.00000060 eV (Source: 2010 CODATA)

real(dp), parameter :: J2Ha = 2.29371248e17_dp  ! 1 J = (1 * J2Ha) Ha
! Standard uncertainty is     0.00000010 eV (Source: 2010 CODATA)

! Reduced Planck constant
real(dp), parameter :: hbar = 1.054571726e-34_dp  ! [J s]
! Standard uncertainty is     0.000000047 eV (Source: 2010 CODATA)

! Covert Ha to cm^{-1} (energy equivalent)
! 1 Ha = (1 * Ha2invcm) cm^{-1}
real(dp), parameter :: Ha2invcm = 219474.6313708_dp
! Standard uncertainty is              0.0000011 cm^{-1} (Source: 2010 CODATA)

real(dp), parameter :: kB = 1.3806488e-23_dp ! Boltzmann constant [J/K]
! Standard uncertainty is   0.0000013 J/K (Source: 2010 CODATA)

real(dp), parameter :: me = 9.10938291e-31_dp  ! electron rest mass [kg]
! Standard uncertainty is   0.00000040 eV (Source: 2010 CODATA)

! Converts K to a.u.:
real(dp), parameter :: K2au = J2Ha*kB ! 1 K = (1 * K2au) a.u.

! Conversion between Bohr (Hartree atomic units) and Angstrom
real(dp), parameter :: bohr2ang = 0.529177249_dp
real(dp), parameter :: ang2bohr = 1 / bohr2ang

! Converts eV to kcal/mol, it is assumed, that the number in eV is given per
! molecule.  1 eV = (1 * eV2kcalmol) kcal/mol
real(dp), parameter :: kcalmol2kJmol = 4.184_dp
real(dp), parameter :: kJmol2Ha = 1000 * J2Ha / Na

! Converts density from a.u. to g/cm^3
real(dp), parameter :: density2gcm3 = me*1e3_dp / (bohr2ang*1e-8_dp)**3

! Converts u (atomic mass unit) to a.u.
real(dp), parameter :: u2au = 1 / (Na * me * 1e3_dp)

! Converts s to a.u.
real(dp), parameter :: s2au = 1 / (J2Ha*hbar)

end module
"""

file3 = """\
module expr1
implicit none

contains

subroutine a
integer :: a, b
a = 1+2*3
b = (1+2+a)*3
end subroutine

end module
"""

file4 = """\
program expr2
implicit none

integer :: x

x = (2+3)*5
print *, x

end program
"""

file5 = """\
module m1
implicit none
private
public sp, dp, hp

integer, parameter :: dp=kind(0.d0), &             ! double precision
                      hp=selected_real_kind(15), & ! high precision
                      sp=kind(0.)                  ! single precision

end module
"""

file6 = """\
program main_prog
implicit none

integer :: x

x = 1
print *, "OK.", x

end program
"""

file7 = """\
module random

use types, only: dp
use utils, only: stop_error
implicit none
private
public randn, rand_gamma

interface randn
    module procedure randn_scalar
    module procedure randn_vector
    module procedure randn_matrix
    module procedure randn_vector_n
end interface

interface rand_gamma
    module procedure rand_gamma_scalar
    module procedure rand_gamma_vector
    module procedure rand_gamma_matrix
    module procedure rand_gamma_vector_n
end interface

contains

subroutine randn_scalar(x)
! Returns a psuedorandom scalar drawn from the standard normal distribution.
!
! [1] Marsaglia, G., & Bray, T. A. (1964). A Convenient Method for Generating
!       Normal Variables. SIAM Review, 6(3), 260-264.
real(dp), intent(out) :: x
logical, save :: first = .true.
real(dp), save :: u(2)
real(dp) :: r2
if (first) then
    do
        call random_number(u)
        u = 2*u-1
        r2 = sum(u**2)
        if (r2 < 1 .and. r2 > 0) exit
    end do
    u = u * sqrt(-2*log(r2)/r2)
    x = u(1)
else
    x = u(2)
end if
first = .not. first
end subroutine

subroutine randn_vector_n(n, x)
integer, intent(in) :: n
real(dp), intent(out) :: x(n)
integer :: i
do i = 1, size(x)
    call randn(x(i))
end do
end subroutine

subroutine randn_vector(x)
real(dp), intent(out) :: x(:)
call randn_vector_n(size(x), x)
end subroutine

subroutine randn_matrix(x)
real(dp), intent(out) :: x(:, :)
call randn_vector_n(size(x), x)
end subroutine

subroutine rand_gamma0(a, first, fn_val)
! Returns a psuedorandom scalar drawn from the gamma distribution.
!
! The shape parameter a >= 1.
!
! [1] Marsaglia, G., & Tsang, W. W. (2000). A Simple Method for Generating
! Gamma Variables. ACM Transactions on Mathematical Software (TOMS), 26(3),
! 363-372.
real(dp), intent(in) :: a
logical, intent(in) :: first
real(dp), intent(out) :: fn_val
real(dp), save :: c, d
real(dp) :: U, v, x
if (a < 1) call stop_error("Shape parameter must be >= 1")
if (first) then
    d = a - 1._dp/3
    c = 1/sqrt(9*d)
end if
do
    do
        call randn(x)
        v = (1 + c*x)**3
        if (v > 0) exit
    end do
    call random_number(U)
    ! Note: the number 0.0331 below is exact, see [1].
    if (U < 1 - 0.0331_dp*x**4) then
        fn_val = d*v
        exit
    else if (log(U) < x**2/2 + d*(1 - v + log(v))) then
        fn_val = d*v
        exit
    end if
end do
end subroutine

subroutine rand_gamma_scalar(a, x)
real(dp), intent(in) :: a
real(dp), intent(out) :: x
call rand_gamma0(a, .true., x)
end subroutine

subroutine rand_gamma_vector_n(a, n, x)
real(dp), intent(in) :: a
integer, intent(in) :: n
real(dp), intent(out) :: x(n)
integer :: i
call rand_gamma0(a, .true., x(1))
do i = 2, size(x)
    call rand_gamma0(a, .false., x(i))
end do
end subroutine

subroutine rand_gamma_vector(a, x)
real(dp), intent(in) :: a
real(dp), intent(out) :: x(:)
call rand_gamma_vector_n(a, size(x), x)
end subroutine

subroutine rand_gamma_matrix(a, x)
real(dp), intent(in) :: a
real(dp), intent(out) :: x(:, :)
call rand_gamma_vector_n(a, size(x), x)
end subroutine

end module
"""

file8 = """\
module subroutine1
implicit none

contains

subroutine some_subroutine
integer :: a, b, c
a = 1
b = 8
end subroutine

subroutine some_other_subroutine
integer :: x, y
y = 5
x = y
call some_subroutine()
end subroutine

end module
"""

file9 = """\
program main_rand
implicit none
integer, parameter :: dp=kind(0.d0)
real(dp) :: a
integer :: i
print *, "Random numbers:"
do i = 1, 10
    call rand(a)
    print *, a
end do

contains

    subroutine rand(x)
    real(dp), intent(out) :: x
    logical, save :: first = .true.
    real(dp), save :: u(2)
    real(dp) :: r2
    if (first) then
        do
            call random_number(u)
            u = 2*u-1
            r2 = sum(u**2)
            if (r2 < 1 .and. r2 > 0) exit
        end do
        u = u * sqrt(-2*log(r2)/r2)
        x = u(1)
    else
        x = u(2)
    end if
    first = .not. first
    end subroutine

end program
"""
