module stdlib_string_type
    implicit none

    type :: string_type
        character(len=:), allocatable :: raw
    end type string_type

    interface string_type

        pure elemental module function new_string(string) result(new)
            character(len=*), intent(in), optional :: string
            type(string_type) :: new
        end function new_string

        pure elemental module function new_string_from_integer_int32(val) result(new)
            integer(4), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int32

        pure elemental module function new_string_from_integer_int64(val) result(new)
            integer(8), intent(in) :: val
            type(string_type) :: new
        end function new_string_from_integer_int64

        pure elemental module function new_string_from_logical_lk(val) result(new)
            logical, intent(in) :: val
            type(string_type) :: new
        end function new_string_from_logical_lk

    end interface string_type

end module stdlib_string_type

submodule(stdlib_string_type) stdlib_string_type_constructor

contains

    elemental module function new_string(string) result(new)
        character(len=*), intent(in), optional :: string
        type(string_type) :: new
    end function new_string

    elemental module function new_string_from_integer_int32(val) result(new)
        integer(4), intent(in) :: val
        type(string_type) :: new
    end function new_string_from_integer_int32

    elemental module function new_string_from_integer_int64(val) result(new)
        integer(8), intent(in) :: val
        type(string_type) :: new
    end function new_string_from_integer_int64

    elemental module function new_string_from_logical_lk(val) result(new)
        logical, intent(in) :: val
        type(string_type) :: new
    end function new_string_from_logical_lk

end submodule stdlib_string_type_constructor

program derived_type_05
end program