program preprocessor7
! undef
implicit none
#define X

#ifdef X
print *, 1
#else
print *, 2
#endif

#undef X

#ifdef X
print *, 3
#else
print *, 4
#endif

#undef X

#ifdef X
print *, 3
#else
print *, 4
#endif

end program
