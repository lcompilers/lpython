module modules_15b
use iso_c_binding, only: c_int, c_float, c_double
implicit none

interface
    ! int f_int_float(int *a, float *b)
    integer(c_int) function f_int_float(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), intent(in) :: a
    real(c_float), intent(in) :: b
    end function

    ! int f_int_double(int *a, double *b)
    integer(c_int) function f_int_double(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    real(c_double), intent(in) :: b
    end function

    ! int f_int_float_value(int a, float b)
    integer(c_int) function f_int_float_value(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    real(c_float), value, intent(in) :: b
    end function

    ! int f_int_double_value(int a, double b)
    integer(c_int) function f_int_double_value(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    end function

    ! int f_int_intarray(int n, int *b)
    integer(c_int) function f_int_intarray(n, b) result(r) bind(c)
    import :: c_int
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: b(n)
    end function

    ! float f_int_floatarray(int n, float *b)
    real(c_float) function f_int_floatarray(n, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: n
    real(c_float), intent(in) :: b(n)
    end function

    ! double f_int_doublearray(int n, double *b)
    real(c_double) function f_int_doublearray(n, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: n
    real(c_double), intent(in) :: b(n)
    end function

    ! int f_int_double_value(int a, double b)
    integer(c_int) function f_int_double_value_name(a, b) result(r) &
            bind(c, name="f_int_double_value")
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    end function
end interface

end module
