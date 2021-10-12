program cpp2
implicit none
#define X123 12345678
integer :: x
x = (2+3)*5
! Error: `yy` not defined:
print *, x, X123, yy
end program
