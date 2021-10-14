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

! more nesting

#ifdef X
print *, "10"
# ifdef Y
  print *, "1a0"
#   ifdef Z
    print *, "1aa"
#   else
    print *, "1ab"
#   endif
  print *, "1ac"
# else
print *, "1b"
# endif
print *, "1c"
#else
print *, "20"
# ifdef Y
  print *, "20"
#   ifdef Z
    print *, "2aa"
#   else
    print *, "2ab"
#   endif
  print *, "2a"
# else
  print *, "2b"
# endif
print *, "2c"
#endif


#ifdef X
print *, "10"
# ifdef Y
  print *, "1a0"
#   ifdef X
    print *, "1aa"
#   else
    print *, "1ab"
#   endif
  print *, "1ac"
# endif
print *, "1c"
#endif

end program
