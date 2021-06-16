program allocate_01
implicit none
integer, allocatable :: a(:), b(:,:), c(:,:,:)
integer :: n, ierr
n = 10
allocate(a(5))
allocate(b(n,n), c(n, 5, n), stat=ierr)
!deallocate(a, c)
end
