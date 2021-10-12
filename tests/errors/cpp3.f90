program cpp3
implicit none
#define X123 12345678
integer :: x, y
x = (2+3)*5
! Error: `xx` not defined:
print *, xx, X123, y
end program
