program preprocessor2
implicit none
#define X123 12345678
#define Y123 X123
#define A123 Y123
#define B123 A123
#define C123 B123
integer :: x, y
x = (2+3)*5
print *, x, C123
end program
