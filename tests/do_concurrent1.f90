subroutine dc1(a, b)
real, intent(inout) :: a(:,:)
integer :: N, i, j
N = size(a)
do concurrent (i=1:N, j=1:N, i<j)
    a(i,j) = a(j,i)
end do
end subroutine

subroutine dc2(a, b)
real, intent(inout) :: a(:), b(:)
integer :: i
real :: x
x = 1.0
do concurrent (i=1:10) local(x)
    if (a(i) > 0) then
        x = sqrt(a(i))
        a(i) = a(i) - x**2
    end if
    b(i) = b(i) - a(i)
end do
print *, x
end subroutine

subroutine dc3(a, b)
real, intent(inout) :: a(:), b(:)
integer :: i
real :: x
x = 1.0
do concurrent (i=1:10) shared(i) local(x) default(none)
    if (a(i) > 0) then
        x = sqrt(a(i))
        a(i) = a(i) - x**2
    end if
    b(i) = b(i) - a(i)
end do
print *, x
end subroutine

subroutine dc4(a, b)
real, intent(inout) :: a(:), b(:)
integer :: i
real :: x
x = 1.0
do concurrent (i=1:10) default(none) local(x) shared(i)
    if (a(i) > 0) then
        x = sqrt(a(i))
        a(i) = a(i) - x**2
    end if
    b(i) = b(i) - a(i)
end do
print *, x
end subroutine
