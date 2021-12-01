module stdlib_logger

    type :: logger_type
        private

        logical                   :: add_blank_line = .false.
        integer, allocatable      :: log_units(:)
        integer                   :: max_width = 0

    contains

        private

        final :: final_logger

    end type logger_type

contains

    subroutine final_logger( self )
                type(logger_type), intent(in) :: self
        
                integer        :: iostat
                character(256) :: message
                integer        :: unit
    end subroutine

end module

program derived_types_05
    implicit none
end program