module a
implicit none

integer, parameter :: r8 = kind(0.d0)

type :: B
end type

type C
end type

type, extends(B), public :: X
    private
    real(r8) :: r1

    type(C), pointer :: cc => null()
    real(r8), pointer :: r2(:) => null(), r3(:) => null()
contains
    procedure :: p1
    procedure :: p2
end type X


contains

subroutine p1(this)
class(X), intent(out) :: this
end subroutine

subroutine p2(this)
class(X), intent(inout) :: this
end subroutine


end module

program derived_types_02
use a, only: X
implicit none
type(X) :: b
end
