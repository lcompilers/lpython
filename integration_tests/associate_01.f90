program associate_01
implicit none
real, allocatable :: a(:), b(:,:), c(:,:,:)
real, pointer :: x(:), y(:,:,:)
integer :: n
n = 10
allocate(a(5))
allocate(b(n,n), c(n, 5, n))
associate (x => a, y => c)
    x(1) = 5
    y(2,3,4) = 3
end associate
end
