module a_01
implicit none

type :: X1
    real :: r
    integer :: i
end type

type :: X2
    real :: r
    integer :: i
end type

contains

subroutine set(a)
type(X1), intent(out) :: a
a%i = 1
a%r = 1.5
end subroutine

end module

program derived_types_01
use a_01, only: X1, X2, set
implicit none
type(X1) :: a
type(X2) :: b
b%i = 5
b%r = 3.5
print *, b%i, b%r
call set(a)
print *, a%i, a%r
end
