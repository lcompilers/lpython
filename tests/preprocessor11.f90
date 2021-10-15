program preprocessor11
! #if with >, <, ==
implicit none

#define X 1
#define Y 5
#if X == 1
print *, "X is 1"
#else
print *, "X is not 1"
#endif

end program
