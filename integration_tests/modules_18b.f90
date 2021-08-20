module modules_18b
use iso_c_binding, only: c_int, c_float, c_double
implicit none

contains

integer function f(a, b) result(r)
integer, intent(in) :: a
real, intent(in) :: b
interface
    ! int f_int_float_value(int a, float b)
    integer(c_int) function f_int_float_value(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    real(c_float), value, intent(in) :: b
    end function
end interface
r = f_int_float_value(a, b)
end function

end module
