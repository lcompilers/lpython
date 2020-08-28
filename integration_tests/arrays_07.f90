program arrays_07
implicit none
integer, dimension(10) :: x
integer :: i
x = [(i, i = 1, size(x))]
print *, x
print *, x(:)
print *, x(3:5)
print *, x(3:)
print *, x(:5)
print *, x(3:5:2)
print *, x(:5:2)
print *, x(3::2)
print *, x(::2)
end program
