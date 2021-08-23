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

subroutine g(a, b, r)
integer, intent(in) :: a
real, intent(in) :: b
integer, intent(out) :: r
interface
    ! void sub_int_float_value(int a, float b, int *r)
    subroutine sub_int_float_value(a, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    real(c_float), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine
end interface
call sub_int_float_value(a, b, r)
end subroutine

end module
