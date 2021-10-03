program intrinsics_21
! Test intrinsics both declarations and executable statements.
! Single and double precision, real only.
integer, parameter :: dp = kind(0.d0)

real :: x1, x2
real(dp) :: y1, y2
integer :: i1, i2

i1 = -5; i2 = 3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(-5., 3.), modulo(-5._dp, 3._dp), modulo(x1, x2), modulo(y1, y2)
print *, mod(-5., 3.), mod(-5._dp, 3._dp), mod(x1, x2), mod(y1, y2)
print *, modulo(-5, 3), modulo(i1, i2)
print *, mod(-5, 3), mod(i1, i2)

i1 = 5; i2 = -3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(5., -3.), modulo(5._dp, -3._dp), modulo(x1, x2), modulo(y1, y2)
print *, mod(5., -3.), mod(5._dp, -3._dp), mod(x1, x2), mod(y1, y2)
print *, modulo(5, -3), modulo(i1, i2)
print *, mod(5, -3), mod(i1, i2)

i1 = -5; i2 = -3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(-5., -3.), modulo(-5._dp, -3._dp), modulo(x1, x2),modulo(y1, y2)
print *, mod(-5., -3.), mod(-5._dp, -3._dp), mod(x1, x2), mod(y1, y2)
print *, modulo(-5, -3), modulo(i1, i2)
print *, mod(-5, -3), mod(i1, i2)

i1 = 5; i2 = 3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(5., 3.), modulo(5._dp, 3._dp), modulo(x1, x2), modulo(y1, y2)
print *, mod(5., 3.), mod(5._dp, 3._dp), mod(x1, x2), mod(y1, y2)
print *, modulo(5, 3), modulo(i1, i2)
print *, mod(5, 3), mod(i1, i2)

end
