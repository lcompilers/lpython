program cpp4
implicit none
#define X123 z12345678
#define Y123 X123
integer :: x, y
x = (2+3)*5
! Error: `z12345678` not defined:
print *, x, Y123
end program
