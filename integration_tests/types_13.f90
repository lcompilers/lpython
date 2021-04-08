program types_13
use iso_c_binding, only: c_int, c_double, c_char
implicit none

interface
    subroutine g(d)
    character*(*) :: d
    end subroutine

    subroutine g2(d)
    character(len=*) :: d
    end subroutine

    subroutine g3(d)
    import :: c_char
    character(len=*, kind=c_char) :: d
    end subroutine

    subroutine g4(d)
    import :: c_char
    character(*, kind=c_char) :: d
    end subroutine

    subroutine g5(d)
    import :: c_char
    character(kind=c_char, len=*) :: d
    end subroutine

    subroutine g6(d)
    character*5 :: d
    end subroutine


    integer(c_int) function f1(a, b, c, d) result(r) bind(c)
    import :: c_int, c_double
    integer*4, value, intent(in) :: a
    real*8, value, intent(in) :: b
    logical*4 :: c
    end function

    integer(c_int) function f2(a, b, c, d) bind(c) result(r)
    import :: c_int, c_double
    integer*4, value, intent(in) :: a
    real*8, value, intent(in) :: b
    logical*4 :: c
    end function

    function f3(a, b, c, d) result(r) bind(c)
    import :: c_int, c_double
    integer(c_int) :: r
    integer*4, value, intent(in) :: a
    real*8, value, intent(in) :: b
    logical*4 :: c
    end function

    function f4(a, b, c, d) bind(c) result(r)
    import c_int, c_double
    integer(c_int) :: r
    integer*4, value, intent(in) :: a
    real*8, value, intent(in) :: b
    logical*4 :: c
    end function

    subroutine f5(a, b, c) bind(C, name="_cf5")
    import :: c_int
    import :: c_double
    integer(c_int) :: r
    integer(c_int), value, intent(in) :: a
    real(c_double), value, intent(in) :: b
    logical :: c
    end subroutine
end interface

end program
