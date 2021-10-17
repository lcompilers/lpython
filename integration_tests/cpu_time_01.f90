program cpu_time_01
use iso_fortran_env, only: dp=>real64
implicit none
real(dp) :: t1, t2
call cpu_time(t1)
print *, "Some computation"
call cpu_time(t2)
print *, "Total time: ", t2-t1
end program
