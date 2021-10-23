program bits_01
use iso_fortran_env, only: int32, int64
implicit none

if (ibclr(3_int32, 0) /= 2_int32) error stop
if (ibclr(4_int64, 1) /= 4_int64) error stop

if (ibset(1_int32, 2) /= 5_int32) error stop
if (ibset(2_int64, 3) /= 10_int64) error stop

if (ieor(16_int32, 2_int32) /= 18_int32) error stop
if (ieor(31_int64, 3_int64) /= 28_int64) error stop

end program
