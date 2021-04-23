program test_kind
use lfortran_intrinsic_kind, only: kind
implicit none
!real :: r4d
!real(4) :: r4
!real(8) :: r8
logical :: l4d
logical(4) :: l4
!if (kind(r4d) /= 4) error stop
!if (kind(r4) /= 4) error stop
!if (kind(r8) /= 8) error stop
if (kind(l4d) /= 4) error stop
if (kind(l4) /= 4) error stop
end program
