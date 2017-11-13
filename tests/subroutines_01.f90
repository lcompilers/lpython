program subroutines_01
implicit none
integer :: i, j
i = 1
j = 1
if (j /= 1) error stop
call f()
if (j /= 2) error stop

contains

    subroutine f()
    j = i + 1
    end subroutine

end program
