subroutine triad(a, b, scalar, c)
real, intent(in) :: a(:), b(:), scalar
real, intent(out) :: c(:)
integer :: N, N2, i, j
N = size(a)
N2 = size(b)
do concurrent (i = 1:N)
    c(i) = a(i) + scalar * b(i)
end do

do concurrent (j = 1:N2)
    c(j) = b(j) + scalar
end do
end subroutine
