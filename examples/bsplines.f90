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
