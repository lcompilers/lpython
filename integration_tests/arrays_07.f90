program arrays_07
implicit none
integer, dimension(10) :: x
integer, dimension(10, 10) :: y
integer :: i, j
integer :: start, end, step
do i = 1, 10
    do j = 1, 10
        y(i, j) = i + j
    end do
end do
x = [(i, i = 1, 10)]
start = x(4)
end = x(7)
step = x(1)
print *, x(start:end:step)
print *, x
print *, x(:)
print *, x(1:5)
print *, x(3:)
print *, x(:5)
print *, x(3:5:2)
print *, x(:5:2)
print *, x(3::2)
print *, x(3: :2)
print *, x(::2)
print *, x(: :2)

print *, y
print *, y(:, 3)
print *, y(3:5, :)
print *, y(4:, 3:)
print *, y(:5, 5:)
print *, y(3:5:2, 3:5:1)
print *, y(:5:2, 6)
print *, y(4, 3::2)
print *, y(3::2, :3:2)
print *, y(::2, ::4)
print *, y(::2, 5)

end program
