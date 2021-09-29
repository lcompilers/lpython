program return_04
implicit none
call sub()

contains

    subroutine sub()
    integer :: n
    if (n <= 1) then
        return
    end if
    if (n == 4) then
        n = 1
    end if
    end subroutine

end program
