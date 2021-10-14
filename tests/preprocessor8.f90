program preprocessor8
! pre-defined macros
implicit none

print *, __LFORTRAN__
#ifdef __VERSION__
print *, "__VERSION__ present"
#endif
print *, __FILE__
print *, __LINE__
print *, __LINE__

end program
