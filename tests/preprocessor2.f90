program preprocessor2
implicit none
#define X123 12345678
#define Y123 X123
integer :: x, y
x = (2+3)*5
print *, x, Y123
end program
