program intrinsics_15
use iso_fortran_env, only: sp=>real32, dp=>real64
integer :: x
real(sp) :: r32
real(dp) :: r64
x = 5; r32 = 5; r64 = 5
! No kind argument
r32 = real(x)
r32 = real(r32)
r32 = real(r64)

! single precision kind
r32 = real(x, sp)
r32 = real(r32, sp)
r32 = real(r64, sp)

! double precision kind
r64 = real(x, dp)
r64 = real(r32, dp)
r64 = real(r64, dp)
end program
