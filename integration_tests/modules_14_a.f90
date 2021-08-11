module modules_14_a
implicit none

interface a
    module procedure a1
    module procedure a2
end interface

contains

    subroutine a1(a)
    integer, intent(inout) :: a
    a = a + 1
    end subroutine

    subroutine a2(a)
    real, intent(inout) :: a
    a = a + 1
    end subroutine

end module
