program preprocessor4
implicit none
#define XPOW(a) ((a)**2)
#define X123(a,b) ((a)+XPOW(b))
integer :: x, y
x = (2+3)*5
print *, x, X123(1,2)
end program
