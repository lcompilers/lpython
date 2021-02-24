program functions_02
implicit none
integer :: i
real :: f, g
i = 1
f = 3.5

g = mysum(i, f)

if (myabs(g-4.5) > 1e-5) error stop

contains

    real function mysum(a, b) result(c)
    integer, intent(in) :: a
    real, intent(in) :: b
    c = a + b
    end function

    real function myabs(a) result(c)
    real, intent(in) :: a
    if (a >= 0) then
        c = a
    else
        c = -a
    end if
    end function

end program
