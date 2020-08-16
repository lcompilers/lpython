program types_07
implicit none

contains

    subroutine f(s)
    character(len=:), allocatable, intent(in) :: s
    end subroutine

    subroutine g(s)
    character(len=*), intent(in) :: s
    end subroutine

end program
