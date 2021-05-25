program arrays_07
implicit none
integer, dimension(10) :: x
integer :: i, start, end, step
x = [(i, i = 1, 10)]
start = x(4)
end = x(7)
step = x(1)
print *, x(start:end:step)
print *, x
print *, x(:)
print *, x(3:5)
print *, x(3:)
print *, x(:5)
print *, x(3:5:2)
print *, x(:5:2)
print *, x(3::2)
print *, x(3: :2)
print *, x(::2)
print *, x(: :2)
end program
