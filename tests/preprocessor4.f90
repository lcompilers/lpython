program preprocessor4
implicit none
#define X123(a,b) a+b
integer :: x, y
x = (2+3)*5
print *, x, X123(1,2)
end program
