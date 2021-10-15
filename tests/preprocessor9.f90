program preprocessor9
! Line continuation
implicit none

#define X 1+2+\
  3 + 4 \
  + 5 + 6

#define Y(x) 1+2+\
  3 + 4*(x) \
  + 5 + 6

print *, X
print *, Y(1+2)

end program
