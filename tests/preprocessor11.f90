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

#if X != 1
print *, "X is not 1"
#else
print *, "Not X is not 1"
#endif

#if X == 1 && Y == 5
print *, "X is 1 and Y is 5"
#else
print *, "Not (X is 1 and Y is 5)"
#endif

#if X == 2 && Y == 5
print *, "X is 2 and Y is 5"
#else
print *, "Not (X is 2 and Y is 5)"
#endif

#if X < 3 && Y > 3
print *, "X<3 and Y>3"
#else
print *, "Not (X<3 and Y>3)"
#endif

#if X > 3 && Y < 3
print *, "X>3 and Y<3"
#else
print *, "Not (X>3 and Y<3)"
#endif

#if X == 3-2
print *, "X is 3-2"
#else
print *, "X is not 3-2"
#endif

#if X == 3-1
print *, "X is 3-1"
#else
print *, "X is not 3-1"
#endif

#if Y == (6*5-5)/5
print *, "Y is (6*5-5)/5"
#else
print *, "Y is not (6*5-5)/5"
#endif

#if Y == (6*5-6)/5
print *, "Y is (6*5-6)/5"
#else
print *, "Y is not (6*5-6)/5"
#endif

end program
