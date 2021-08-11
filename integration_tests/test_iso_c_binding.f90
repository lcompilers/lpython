program test_iso_c_binding
use iso_c_binding, only: c_long, c_long_long, c_float, c_double
implicit none
real(c_float) :: r4
real(c_double) :: r8
integer(c_long) :: i4
integer(c_long_long) :: i8
!if (kind(r4) /= 4) error stop
!if (kind(r8) /= 8) error stop
!if (kind(i4) /= 4) error stop
!if (kind(i8) /= 8) error stop
end program
