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

#if X == 2
print *, "X is 2"
#else
print *, "X is not 2"
#endif

end program
