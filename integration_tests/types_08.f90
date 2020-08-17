program types_08
implicit none

contains

    subroutine f(i, j)
    integer, optional, intent(in) :: i
    integer, intent(in) :: j
    optional :: j
    end subroutine

    subroutine g(i, j)
    integer, optional, intent(in) :: i
    integer j
    intent(in) j
    optional j
    end subroutine

end program
