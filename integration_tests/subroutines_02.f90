program subroutines_02
implicit none
integer :: i, j
i = 1
j = 1
call f(i, j)
if (i /= 1) error stop
if (j /= 2) error stop

call g(i, j)
if (i /= 1) error stop
if (j /= 0) error stop

call h(i, j)
if (i /= 1) error stop
if (j /= 0) error stop

contains

    subroutine f(a, b)
    integer, intent(in) :: a
    integer, intent(out) :: b
    b = a + 1
    end subroutine

    subroutine g(a, b)
    integer, intent(in) :: a
    integer, intent(out) :: b
    b = a - 1
    end subroutine

    subroutine h(a, b)
    integer, intent(in) :: a
    integer, intent(out) :: b
    call g(a, b)
    end subroutine

end program
