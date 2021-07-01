module interface_04_mod1
implicit none

interface a
    module procedure a1
end interface

contains

    subroutine a1(a)
    integer, intent(inout) :: a
    a = a + 1
    end subroutine

end module

module interface_04_mod2
use interface_04_mod1
end module

program interface_04
use interface_04_mod2, only: a
implicit none

integer :: i

i = 5
call a(i)
if (i /= 6) error stop

end program
