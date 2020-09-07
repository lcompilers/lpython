program types_13
use iso_c_binding, only: c_int, c_double
implicit none

interface
    integer(c_int) function f(a, b, c, d) result(r)
    import :: c_int, c_double
    integer*4, value, intent(in) :: a
    real*8, value, intent(in) :: b
    logical*4 :: c
    character*(*) :: d
    end function
end interface

end program
