program kwargs_01
! Test keyword arguments
implicit none

integer :: i

i = f(1, 2, 3, 4)
if (i /= 1234) error stop
call s(1, 2, 3, 4, i)
if (i /= 1234) error stop

i = f(1, 2, 3, d=4)
if (i /= 1234) error stop
call s(1, 2, 3, d=4, f=i)
if (i /= 1234) error stop

i = f(1, 2, c=3, d=4)
if (i /= 1234) error stop
call s(1, 2, c=3, d=4, f=i)
if (i /= 1234) error stop

i = f(1, 2, d=3, c=4)
if (i /= 1243) error stop
call s(1, 2, d=3, c=4, f=i)
if (i /= 1243) error stop

i = f(1, b=2, c=3, d=4)
if (i /= 1234) error stop
call s(1, b=2, c=3, d=4, f=i)
if (i /= 1234) error stop

i = f(1, c=2, d=3, b=4)
if (i /= 1423) error stop
call s(1, c=2, d=3, b=4, f=i)
if (i /= 1423) error stop

i = f(a=1, b=2, c=3, d=4)
if (i /= 1234) error stop
call s(a=1, b=2, c=3, d=4, f=i)
if (i /= 1234) error stop

i = f(d=1, c=2, a=3, b=4)
if (i /= 3421) error stop
call s(d=1, c=2, a=3, b=4, f=i)
if (i /= 3421) error stop

contains

    integer function f(a, b, c, d)
    integer, intent(in) :: a, b, c, d
    f = a*1000 + b*100 + c*10 + d
    end function

    subroutine s(a, b, c, d, f)
    integer, intent(in) :: a, b, c, d
    integer, intent(out) :: f
    f = a*1000 + b*100 + c*10 + d
    end subroutine

end program
