program arrays_06
implicit none
real, dimension(5) :: x
integer :: i
x = [(i*2, i = 1, 5)]
print *, x
end program
