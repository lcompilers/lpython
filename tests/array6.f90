program array6
implicit none
contains
    subroutine t(a, b, c, d)
    real :: a(*), b(3:*)
    real, dimension(*) :: c
    real, dimension(4:*) :: d
    end subroutine
end program
