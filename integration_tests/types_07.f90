program types_07
implicit none
integer, target :: x
integer, pointer :: y => x
real, target :: a(5)
real, pointer :: b(:) => a

contains

    subroutine f(s, s2)
    character(len=:), allocatable, intent(in) :: s
    character(:), allocatable, intent(in) :: s2
    end subroutine

    subroutine g(s, s2)
    character(len=*), intent(in) :: s
    character(*), intent(in) :: s2
    end subroutine

end program
