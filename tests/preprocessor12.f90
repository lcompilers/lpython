#include "assert.fpp"

program preprocessor12
! Test that a typical ASSERT macro works
implicit none

ASSERT(.true.)
ASSERT(5 > 3)
ASSERT(5+3*8 > 3)

ASSERT(fn(3, 5))

ASSERT(5 < 3)

contains

    logical function fn(a, b)
    integer, intent(in) :: a, b
    fn = a < b
    end function

    subroutine f90_assert(file, line)
    character(len=*), intent(in) :: file
    integer,          intent(in) :: line
    print *, "Assertion failed at " // file // ":", line
    error stop
    end subroutine f90_assert

end program
