module a
implicit none

type :: X
    integer :: i
end type

end module

program derived_types_01
use a, only: X
implicit none
type(X) :: b
end
