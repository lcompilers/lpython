program arrays_op_7
implicit none

integer :: x(3), y(3)

x = 3
call f(x, y)
print *, y

contains

    subroutine f(x, y)
    integer, intent(in) :: x(:)
    integer, intent(out) :: y(:)
    y = x
    end subroutine

end program
