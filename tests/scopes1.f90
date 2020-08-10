program scopes1
implicit none
integer :: i, j
j = 1
call f(i)

contains

    subroutine f(b)
    integer, intent(out) :: b
    b = j + 1
    end subroutine

end program
