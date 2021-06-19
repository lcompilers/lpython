program arrays_07
implicit none
integer, dimension(10) :: x
integer, dimension(3, 3) :: y
integer :: i, j
! integer :: start, end, step
do i = 1, 3
    do j = 1, 3
        y(i, j) = i + j
    end do
end do
x = [(i, i = 1, 10)]
! start = x(4)
! end = x(7)
! step = x(1)
! print *, x(start:end:step)
print *, x
print *, x(1:5)
print *, x(3:)
print *, x(:5)
print *, x(5)
print *, x(3:5:2)
print *, x(:5:2)
print *, x(3::2)
print *, x(3: :2)
print *, x(::2)
print *, x(: :2)

print *, y
print *, y(:, 3)
print *, y(2:, :)
print *, y(4:, 3:)
print *, y(:2, 2:)
print *, y(1:2:2, 1:2:1)
print *, y(:3:2, 3)
print *, y(3, 3::2)
print *, y(3::2, :3:2)
print *, y(::2, ::4)
print *, y(::2, 3)

end program
