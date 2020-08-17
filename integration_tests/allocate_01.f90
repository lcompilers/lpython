program allocate_01
implicit none
real, allocatable :: a(:), b(:,:), c(:,:,:)
integer :: n
n = 10
allocate(a(5))
allocate(b(n,n), c(n, 5, n))
deallocate(a, c)
end
