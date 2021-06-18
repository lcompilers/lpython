program allocate_01
implicit none
integer, allocatable :: a(:), b(:,:), c(:,:,:)
integer :: n, ierr
integer :: i, j, k
n = 10
allocate(a(5))
allocate(b(n + 1,n), c(n, 5, n), stat=ierr)
do i = 1, 5
    a(i) = i
end do
do i = 1, n
    do j = 1, n
        b(i, j) = i + j
    end do
end do
do i = 1, n
    do j = 1, 5
        do k = 1, n
            c(i, j, k) = i + j + k
        end do
    end do
end do

do i = 1, 5
    if (a(i) /= i) error stop
end do
do i = 1, n
    do j = 1, n
        if (b(i, j) /= i + j) error stop
    end do
end do
do i = 1, n
    do j = 1, 5
        do k = 1, n
            if (c(i, j, k) /= i + j + k) error stop
        end do
    end do
end do
!deallocate(a, c)
end
