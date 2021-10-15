program preprocessor9
! Line continuation
implicit none

#define X 1+2+\
  3 + 4 \
  + 5 + 6

print *, X

end program
