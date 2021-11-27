module stdlib_string_type
    type :: string_type
        sequence
        private
        character(len=:), allocatable :: raw
    end type string_type

    interface write(formatted)
        module procedure :: write_formatted
    end interface

    interface read(formatted)
        module procedure :: read_formatted
    end interface

contains

subroutine write_formatted(string, unit, iotype, v_list, iostat, iomsg)
    type(string_type), intent(in) :: string
    integer, intent(in) :: unit
    character(len=*), intent(in) :: iotype
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(len=*), intent(inout) :: iomsg
end subroutine write_formatted

subroutine read_formatted(string, unit, iotype, v_list, iostat, iomsg)
    type(string_type), intent(inout) :: string
    integer, intent(in) :: unit
    character(len=*), intent(in) :: iotype
    integer, intent(in) :: v_list(:)
    integer, intent(out) :: iostat
    character(len=*), intent(inout) :: iomsg
    character(len=:), allocatable :: line
end subroutine read_formatted

end module

program string_14
end program