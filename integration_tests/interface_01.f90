module interface_01_mod
implicit none

interface a
    module procedure a1
    module procedure a2
end interface

contains

    subroutine a1(a)
    integer, intent(inout) :: a
    a = a + 1
    end subroutine

    subroutine a2(a)
    real, intent(in out) :: a
    a = a + 1
    end subroutine

end module

program interface_01
use interface_01_mod, only: a
implicit none

integer :: i
real :: r

i = 5
call a(i)
if (i /= 6) error stop

r = 6
call a(r)
if (r /= 7) error stop

i = 7
call a(i)
if (i /= 8) error stop

end program
