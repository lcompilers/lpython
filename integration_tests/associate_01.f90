program associate_01
implicit none
real, allocatable :: a(:), b(:,:), c(:,:,:)
real, pointer :: x(:), y(:,:,:)
real :: a_1, c_234
integer :: n
a_1 = 0
n = 10
allocate(a(5))
allocate(b(n,n), c(n, 5, n))
1 loop: associate (x => a, y => c)
    x(1) = a_1 + 5
    y(2,3,4) = 3
end associate loop
a_1 = a(1)
if(a(1).EQ.5) GO TO 1
c_234 = c(2,3,4)
print *, a_1
print *, c_234
end
