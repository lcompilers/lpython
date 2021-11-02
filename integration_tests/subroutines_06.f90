module bitset
    interface assignment(=)
        pure module subroutine assign_large( set1, set2 )
            integer, intent(out) :: set1
            integer, intent(in)  :: set2
        end subroutine assign_large

        pure module subroutine assign_logint8_large( self, logical_vector )
            integer, intent(out) :: self
            logical, intent(in)       :: logical_vector(:)
        end subroutine assign_logint8_large
    end interface

    interface error_handler
        module subroutine error_handler( message, error, status, &
            module, procedure )
            character(*), intent(in)           :: message
            integer, intent(in)                :: error
            integer, intent(out), optional     :: status
            character(*), intent(in), optional :: module
            character(*), intent(in), optional :: procedure
        end subroutine error_handler
    end interface error_handler

contains

    module subroutine error_handler( message, error, status, module, procedure )
        character(*), intent(in)           :: message
        integer, intent(in)                :: error
        integer, intent(out), optional     :: status
        character(*), intent(in), optional :: module
        character(*), intent(in), optional :: procedure

        print *, error, status
    end subroutine error_handler

end module

program subroutines_06

    use bitset
    implicit none

    ! empty program

end program subroutines_06