program preprocessor1
implicit none
#define X123 12345678
integer :: x, y
x = (2+3)*5
print *, x, X123
end program
