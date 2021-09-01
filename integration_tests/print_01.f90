program print_01
implicit none
integer :: x
x = (2+3)*5
print *, x, 1, 3, x, (2+3)*5+x
write(*,*) x, 1, 3, x, (2+3)*5+x
end program
