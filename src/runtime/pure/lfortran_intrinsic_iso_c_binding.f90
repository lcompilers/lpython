module lfortran_intrinsic_iso_c_binding
implicit none

integer, parameter :: c_int = 4
integer, parameter :: c_long = 4
integer, parameter :: c_long_long = 8
integer, parameter :: c_size_t = 8
integer, parameter :: c_float = 4
integer, parameter :: c_double = 8
integer, parameter :: c_bool = 4
integer, parameter :: c_char = 1
character(len=1), parameter :: c_null_char = char(0)

type :: c_ptr
    integer ptr
end type

interface
    logical function c_associated(c_ptr_1)
    import c_ptr
    type(c_ptr), intent(in) :: c_ptr_1
    end function

    subroutine c_f_pointer(cptr, fptr)
    import c_ptr
    type(c_ptr), intent(in) :: cptr
    !type(*), pointer, intent(out) :: fptr
    integer, pointer, intent(out) :: fptr
    end subroutine
end interface

end module
