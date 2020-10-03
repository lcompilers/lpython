program parameter_01
implicit none
integer :: i, j, k
parameter(i = 1)
parameter(j=2, k=3)
print *, i, j, k
end program
