program allocate_01
implicit none
integer, allocatable :: a(:)
real, allocatable :: b(:, :) 
complex, allocatable :: c(:, :, :)
complex :: r
integer :: n, ierr
integer :: i, j, k
n = 10
allocate(a(5:n + 5))
allocate(b(n:2*n, n:3*n), c(5:n + 5, n:2*n, n:3*n), stat=ierr)
if( size(a) /= n + 1 ) error stop
if( size(b) /= (n + 1)*(2*n + 1) ) error stop
if( size(c) /= ((n + 1)**2)*(2*n + 1) ) error stop
do i = lbound(a, 1), ubound(a, 1)
    a(i) = i
end do
do i = lbound(b, 1), ubound(b, 1)
    do j = lbound(b, 2), ubound(b, 2)
        b(i, j) = i + j
    end do
end do

call sum(a, b, c)

do i = lbound(a, 1), ubound(a, 1)
    if (a(i) /= i) error stop
end do
do i = lbound(b, 1), ubound(b, 1)
    do j = lbound(b, 2), ubound(b, 2)
        if (b(i, j) /= i + j) error stop
    end do
end do
do i = lbound(c, 1), ubound(c, 1)
    do j = lbound(c, 2), ubound(c, 2)
        do k = lbound(c, 3), ubound(c, 3)
            if (c(i, j, k) /= i + j + k) error stop
        end do
    end do
end do

r = reduce_sum(c)
if (r /= (114345.0, 0.0)) error stop

deallocate(b)
deallocate(a, c)

contains

subroutine sum(a, b, c)
implicit none

integer, intent(in) :: a(:)
real, intent(in) :: b(:, :)
complex, intent(out) :: c(:, :, :)
integer :: i, j, k

do i = lbound(a, 1), ubound(a, 1)
    do j = lbound(b, 1), ubound(b, 1)
        do k = lbound(b, 2), ubound(b, 2)
            c(i, j, k) = a(i) + b(j, k)
        end do
    end do
end do

end subroutine sum

complex function reduce_sum(c) result(r)
implicit none

complex, intent(in) :: c(:, :, :)
integer :: i, j, k

r = 0

do i = lbound(c, 1), ubound(c, 1)
    do j = lbound(c, 2), ubound(c, 2)
        do k = lbound(c, 3), ubound(c, 3)
            r = r + c(i, j, k)
        end do
    end do
end do

end function reduce_sum

end
