module a_01
implicit none

type :: X
    integer :: i
end type

end module

program derived_types_01
use a_01, only: X
implicit none
type(X) :: b
end
