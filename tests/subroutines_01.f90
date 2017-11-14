program subroutines_01
implicit none
integer :: i, j
i = 1
j = 1
if (j /= 1) error stop
call f(i, j)
if (j /= 2) error stop

contains

    subroutine f(a, b)
    integer, intent(in) :: a
    integer, intent(out) :: b
    b = a + 1
    end subroutine

end program
