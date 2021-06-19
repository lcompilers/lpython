program allocate_01
implicit none
integer, allocatable :: a(:), b(:,:), c(:,:,:)
integer :: n, ierr
integer :: i, j, k
n = 10
allocate(a(n + 5))
allocate(b(2*n, 3*n), c(n, n, 10), stat=ierr)
if( size(a) /= n + 5 ) error stop
if( size(b) /= 2*n*3*n ) error stop
if( size(c) /= n*n*10 ) error stop
do i = lbound(a, 1), ubound(a, 1)
    a(i) = i
end do
do i = lbound(b, 1), ubound(b, 1)
    do j = lbound(b, 2), ubound(b, 2)
        b(i, j) = i + j
    end do
end do
do i = lbound(c, 1), ubound(c, 1)
    do j = lbound(c, 2), ubound(c, 2)
        do k = lbound(c, 3), ubound(c, 3)
            c(i, j, k) = i + j + k
        end do
    end do
end do

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
!deallocate(a, c)
end
