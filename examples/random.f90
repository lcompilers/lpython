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
!       Normal Variables. SIAM Review, 6(3), 260–264.
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
! 363–372.
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
