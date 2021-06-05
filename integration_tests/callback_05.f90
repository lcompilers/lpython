module callback_05
implicit none

contains

subroutine px_call3(f, x)
integer, intent(in) :: x
interface
    subroutine f(x)
        integer, intent(in) :: x
    end subroutine
end interface
call f(x)
end subroutine

subroutine px_call2(f, x)
integer, intent(in) :: x
interface
    subroutine f(x)
        integer, intent(in) :: x
    end subroutine
end interface
call px_call3(f, x)
end subroutine

subroutine px_call1(x)
integer, intent(in) :: x
call px_call2(printx, x)

contains

    subroutine printx(x)
        integer, intent(in) :: x
        print *, x
    end subroutine

end subroutine

end module callback_05

program main
use callback_05
implicit none
integer :: x = 5
call px_call1(x)
end program main
