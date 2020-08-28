program types_10
implicit none

integer :: i, j

save i
save :: j

contains

    subroutine f(i)
    integer, intent(in) :: i
    integer, save :: j
    j = i
    end subroutine

    subroutine g(i)
    integer, intent(in) :: i
    integer :: j
    save j
    j = i
    end subroutine

    subroutine h(i)
    integer, intent(in) :: i
    integer :: j
    save
    j = i
    end subroutine

end program
