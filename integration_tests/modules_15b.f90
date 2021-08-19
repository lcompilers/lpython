module modules_15b
use iso_c_binding, only: c_int, c_float, c_double
implicit none

interface
    ! int f1(int *a, float *b)
    integer(c_int) function f1(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), intent(in) :: a
    real(c_float), intent(in) :: b
    end function

    ! int f2(int *a, double *b)
    integer(c_int) function f2(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    real(c_double), intent(in) :: b
    end function

    ! double f3(int *n, double *b)
    real(c_double) function f3(n, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: n
    real(c_double), intent(in) :: b(n)
    end function

    ! int f4(int *, double b)
    integer(c_int) function f4(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    end function
end interface

end module
