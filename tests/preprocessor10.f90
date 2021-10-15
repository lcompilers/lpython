program preprocessor10
! #if
implicit none

#define X
#define Y(x) 4*(x)

#if defined(X)
print *, "X is defined"
#endif

#if defined( Y )
print *, "Y is defined"
#else
print *, "Y is not defined"
#endif

#if defined(Z)
print *, "Z is defined"
#else
print *, "Z is not defined"
#endif

#if defined(X) && defined(Y)
print *, "X and Y is defined"
#else
print *, "X and Y is not defined"
#endif

#if defined(Z) && defined(Y)
print *, "Z and Y is defined"
#else
print *, "Z and Y is not defined"
#endif

#if defined(Z) || defined(Y)
print *, "Z or Y is defined"
#else
print *, "Z or Y is not defined"
#endif

#if defined(X) && defined(Y) && defined(Z)
print *, "X and Y and Z is defined"
#else
print *, "X and Y and Z is not defined"
#endif

#if defined(X) && defined(Y) || defined(Z)
print *, "X and Y or Z is defined"
#else
print *, "X and Y or Z is not defined"
#endif

#if defined(X) &&    \
    defined(Y) || defined(Z)
print *, "X and Y or Z is defined"
#else
print *, "X and Y or Z is not defined"
#endif

#if defined(X) &&    \
    !defined(Y) || !defined(Z)
print *, "X and !Y or !Z is defined"
#else
print *, "X and !Y or !Z is not defined"
#endif

#if defined(X) &&    \
    !(defined(Y) || defined(Z))
print *, "X and !(Y or Z) is defined"
#else
print *, "X and !(Y or Z) is not defined"
#endif

end program
