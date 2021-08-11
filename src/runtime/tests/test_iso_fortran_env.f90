program test_iso_fortran_env
use lfortran_intrinsic_iso_fortran_env, only: int32, int64, real32, real64
implicit none
real(real32) :: r4
real(real64) :: r8
integer(int32) :: i4
integer(int64) :: i8
if (kind(r4) /= 4) error stop
if (kind(r8) /= 8) error stop
if (kind(i4) /= 4) error stop
if (kind(i8) /= 8) error stop
end program
