module modules_15b
use iso_c_binding, only: c_int, c_double
implicit none

interface
    ! int f(int a, double b)
    integer(c_int) function f(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    real(c_double), intent(in) :: b
    end function
end interface

end module
