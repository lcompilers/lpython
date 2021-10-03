program intrinsics_21
! Test mod / modulo intrinsics both declarations and executable statements.
! Single and double precision, real only.
integer, parameter :: dp = kind(0.d0)

real, parameter :: &
    s1 = modulo(-5., 3.), &
    s2 = modulo(5., -3.), &
    s3 = modulo(-5., -3.), &
    s4 = modulo(5., 3.), &
    s1b = mod(-5., 3.), &
    s2b = mod(5., -3.), &
    s3b = mod(-5., -3.), &
    s4b = mod(5., 3.)
real(dp), parameter :: &
    d1 = modulo(-5._dp, 3._dp), &
    d2 = modulo(5._dp, -3._dp), &
    d3 = modulo(-5._dp, -3._dp), &
    d4 = modulo(5._dp, 3._dp), &
    d1b = mod(-5._dp, 3._dp), &
    d2b = mod(5._dp, -3._dp), &
    d3b = mod(-5._dp, -3._dp), &
    d4b = mod(5._dp, 3._dp)
integer, parameter :: &
    j1 = modulo(-5, 3), &
    j2 = modulo(5, -3), &
    j3 = modulo(-5, -3), &
    j4 = modulo(5, 3), &
    j1b = mod(-5, 3), &
    j2b = mod(5, -3), &
    j3b = mod(-5, -3), &
    j4b = mod(5, 3)

real :: x1, x2
real(dp) :: y1, y2
integer :: i1, i2

i1 = -5; i2 = 3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(-5., 3.), modulo(-5._dp, 3._dp), modulo(x1, x2), modulo(y1, y2), s1, d1
print *, mod(-5., 3.), mod(-5._dp, 3._dp), mod(x1, x2), mod(y1, y2), s1b, d1b
print *, modulo(-5, 3), modulo(i1, i2), j1
print *, mod(-5, 3), mod(i1, i2), j1b

i1 = 5; i2 = -3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(5., -3.), modulo(5._dp, -3._dp), modulo(x1, x2), modulo(y1, y2), s2, d2
print *, mod(5., -3.), mod(5._dp, -3._dp), mod(x1, x2), mod(y1, y2), s2b, d2b
print *, modulo(5, -3), modulo(i1, i2), j2
print *, mod(5, -3), mod(i1, i2), j2b

i1 = -5; i2 = -3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(-5., -3.), modulo(-5._dp, -3._dp), modulo(x1, x2),modulo(y1, y2), s3, d3
print *, mod(-5., -3.), mod(-5._dp, -3._dp), mod(x1, x2), mod(y1, y2), s3b, d3b
print *, modulo(-5, -3), modulo(i1, i2), j3
print *, mod(-5, -3), mod(i1, i2), j3b

i1 = 5; i2 = 3
x1 = i1; y1 = i1;
x2 = i2; y2 = i2;
print *, modulo(5., 3.), modulo(5._dp, 3._dp), modulo(x1, x2), modulo(y1, y2), s4, d4
print *, mod(5., 3.), mod(5._dp, 3._dp), mod(x1, x2), mod(y1, y2), s4b, d4b
print *, modulo(5, 3), modulo(i1, i2), j4
print *, mod(5, 3), mod(i1, i2), j4b

end
