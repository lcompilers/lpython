program intrinsics_22
! Test min/max intrinsics both declarations and executable statements.
! Single and double precision, real only.
integer, parameter :: dp = kind(0.d0)

real, parameter :: &
    s1 = min(-5., 3.), &
    s1b = max(-5., 3.)
real(dp), parameter :: &
    d1 = min(-5._dp, 3._dp), &
    d1b = max(-5._dp, 3._dp)
integer, parameter :: &
    j1 = min(-5, 3), &
    j1b = max(-5, 3)

real :: x1, x2
real(dp) :: y1, y2
integer :: i1, i2

i1 = -5; i2 = 3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, min(-5., 3.), min(-5._dp, 3._dp), min(x1, x2), min(y1, y2), s1, d1
print *, max(-5., 3.), max(-5._dp, 3._dp), max(x1, x2), max(y1, y2), s1b, d1b
print *, min(-5, 3), min(i1, i2), j1
print *, max(-5, 3), max(i1, i2), j1b

end
