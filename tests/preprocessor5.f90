program preprocessor5
implicit none
#define X 5
#define Y

#ifdef X
print *, 1
#else
print *, 2
#endif

#ifdef Y
print *, 3
#else
print *, 4
#endif

#ifdef Z
print *, 5
#else
print *, 6
#endif
end program
