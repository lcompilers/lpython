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

    ! int f_int_float_complex(int *a, float_complex_t *b)
    integer(c_int) function f_int_float_complex(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), intent(in) :: a
    complex(c_float), intent(in) :: b
    end function

    ! int f_int_double_complex(int *a, double_complex_t *b)
    integer(c_int) function f_int_double_complex(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    complex(c_double), intent(in) :: b
    end function

    ! int f_int_float_complex_value(int a, float_complex_t b)
    integer(c_int) function f_int_float_complex_value(a, b) result(r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    complex(c_float), value, intent(in) :: b
    end function

    ! int f_int_double_complex_value(int a, double_complex_t b)
    integer(c_int) function f_int_double_complex_value(a, b) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    complex(c_double), value, intent(in) :: b
    end function

    ! float_complex_t f_float_complex_value_return(float_complex_t b)
    complex(c_float) function f_float_complex_value_return(b) result(r) bind(c)
    import :: c_float
    complex(c_float), value, intent(in) :: b
    end function

    ! double_complex_t f_double_complex_value_return(double_complex_t b)
    complex(c_double) function f_double_complex_value_return(b) result(r)bind(c)
    import :: c_double
    complex(c_double), value, intent(in) :: b
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

!----------------------------------------------------------------------------

    ! void sub_int_float(int *a, float *b, int *r)
    subroutine sub_int_float(a, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), intent(in) :: a
    real(c_float), intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_double(int *a, double *b, int *r)
    subroutine sub_int_double(a, b, r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    real(c_double), intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_float_complex(int *a, float_complex_t *b, int *r)
    subroutine sub_int_float_complex(a, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), intent(in) :: a
    complex(c_float), intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_double_complex(int *a, double_complex_t *b, int *r)
    subroutine sub_int_double_complex(a, b, r) bind(c)
    import :: c_int, c_double
    integer(c_int), intent(in) :: a
    complex(c_double), intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_float_value(int a, float b, int *r)
    subroutine sub_int_float_value(a, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    real(c_float), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_double_value(int a, double b, int *r)
    subroutine sub_int_double_value(a, b, r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_float_complex_value(int a, float_complex_t b, int *r)
    subroutine sub_int_float_complex_value(a, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: a
    complex(c_float), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_double_complex_value(int a, double_complex_t b, int *r)
    subroutine sub_int_double_complex_value(a, b, r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    complex(c_double), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_intarray(int n, int *b, int *r)
    subroutine sub_int_intarray(n, b, r) bind(c)
    import :: c_int
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: b(n)
    integer(c_int), intent(out) :: r
    end subroutine

    ! void sub_int_floatarray(int n, float *b, float *r)
    subroutine sub_int_floatarray(n, b, r) bind(c)
    import :: c_int, c_float
    integer(c_int), value, intent(in) :: n
    real(c_float), intent(in) :: b(n)
    real(c_float), intent(out) :: r
    end subroutine

    ! void sub_int_doublearray(int n, double *b, double *r)
    subroutine sub_int_doublearray(n, b, r) bind(c)
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: n
    real(c_double), intent(in) :: b(n)
    real(c_double), intent(out) :: r
    end subroutine

    ! void sub_int_double_value(int a, double b, int *r)
    subroutine sub_int_double_value_name(a, b, r) &
        bind(c, name="sub_int_double_value")
    import :: c_int, c_double
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    integer(c_int), intent(out) :: r
    end subroutine

end interface

end module
