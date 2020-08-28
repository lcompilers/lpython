program interface_03
implicit none

interface
    subroutine a1(a)
    integer, intent(inout) :: a
    end subroutine
end interface

interface
    subroutine a2(a)
    integer, intent(inout) :: a
    end subroutine

    subroutine a3(a)
    real, intent(inout) :: a
    end subroutine
end interface

procedure(a1) :: sub

contains

    subroutine X()
    interface
        subroutine a4(a)
        integer, intent(inout) :: a
        end subroutine
    end interface
    end subroutine

end program
