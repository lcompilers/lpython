program arrays_06
implicit none
real, dimension(6) :: x
integer :: i
x = [(i*2, i = 1, 6)]
print *, x
x = [(i+1, i*2, i = 1, 3)]
print *, x
x = [(i+1, i**2, i*2, i = 1, 2)]
print *, x
x = [(2*i, 3*i, 4*i, i+1, i**2, i*2, i = 2, 2)]
print *, x
end program
