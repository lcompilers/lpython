module interface_02_mod
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
endinterface

end module

program interface_02
use interface_02_mod, only: a1, a2, a3
implicit none

end program
