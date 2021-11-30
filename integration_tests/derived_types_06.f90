module abstract_type
    implicit none

    contains

    subroutine subrout(a)
    class(*), intent(out) :: a
    end subroutine

    end module

    program derived_types_01
end program