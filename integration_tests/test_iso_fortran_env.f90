program test_iso_fortran_env
use iso_fortran_env, only: int32, int64, real32, real64
implicit none
real(real32) :: r4
real(real64) :: r8
integer(int32) :: i4
integer(int64) :: i8
real(real32), parameter :: r4p = 1._real32
integer(int32), parameter :: i4p = 1_int32
r4 = 1._real32
r8 = 1._real64
i4 = 1_int32
i8 = 1_int64
!if (kind(r4) /= 4) error stop
!if (kind(r8) /= 8) error stop
!if (kind(i4) /= 4) error stop
!if (kind(i8) /= 8) error stop
end program
