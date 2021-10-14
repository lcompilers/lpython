program preprocessor5
implicit none
#define X 5
#ifdef X
print *, 1
#else
print *, 2
#endif
end program
