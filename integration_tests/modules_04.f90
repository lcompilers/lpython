module a_04
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

program modules_04
use, intrinsic :: iso_fortran_env
implicit none

call f()

contains

    subroutine f()
    use a_04, only: b
    call b()
    end subroutine

    integer function g()
    use :: a_04, only: b
    call b()
    g = 5
    end function

end
