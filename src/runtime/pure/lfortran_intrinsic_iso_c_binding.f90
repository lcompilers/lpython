module lfortran_intrinsic_iso_c_binding
implicit none

integer, parameter :: c_int = 4
integer, parameter :: c_long = 4
integer, parameter :: c_long_long = 8
integer, parameter :: c_float = 4
integer, parameter :: c_double = 8
integer, parameter :: c_bool = 4
integer, parameter :: c_char = 1
character(len=1), parameter :: c_null_char = char(0)

end module
