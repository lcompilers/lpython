program preprocessor6
! Nested macros
implicit none
#define X 5
#define Y

#ifdef X
# ifdef Y
print *, "1a"
# else
print *, "1b"
# endif
#else
# ifdef Y
print *, "2a"
# else
print *, "2b"
# endif
#endif

#ifdef X
# ifdef Z
print *, "1a"
# else
print *, "1b"
# endif
#else
# ifdef Z
print *, "2a"
# else
print *, "2b"
# endif
#endif

#ifdef Z
# ifdef Y
print *, "1a"
# else
print *, "1b"
# endif
#else
# ifdef Y
print *, "2a"
# else
print *, "2b"
# endif
#endif

#ifdef Z
# ifdef Z
print *, "1a"
# else
print *, "1b"
# endif
#else
# ifdef Z
print *, "2a"
# else
print *, "2b"
# endif
#endif

! ifndef
#ifdef X
# ifndef Y
print *, "1a"
# else
print *, "1b"
# endif
#else
# ifndef Y
print *, "2a"
# else
print *, "2b"
# endif
#endif

end program
