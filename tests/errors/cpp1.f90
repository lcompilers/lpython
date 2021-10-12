program cpp1
implicit none
#define X123 z12345678
integer :: x, y
x = (2+3)*5
! Error: `z12345678` not defined:
print *, x, X123, y
end program
