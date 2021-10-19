program bits_01
use iso_fortran_env, only: int32, int64
implicit none

if (ibclr(3_int32, 0) /= 2_int32) error stop
if (ibclr(4_int64, 1) /= 4_int64) error stop
end program
