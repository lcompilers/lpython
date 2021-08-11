module interface_07_mod
implicit none

interface a
    module procedure a1
    module procedure a2
end interface

contains

    integer function a1(a)
    integer, intent(in) :: a
    a1 = a + 1
    end function

    real function a2(a)
    real, intent(in) :: a
    a2 = a + 1
    end function

    subroutine run()
    integer :: i
    real :: r

    i = 5
    i = a(i)
    if (i /= 6) error stop

    r = 6
    r = a(r)
    if (r /= 7) error stop

    i = 7
    i = a(i)
    if (i /= 8) error stop
    end subroutine

end module

program interface_07
use interface_07_mod, only: run
implicit none

call run()

end program
