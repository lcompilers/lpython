program arrays_04_func
implicit none
real :: a(3), b
a(1) = 3
a(2) = 2
a(3) = 1
b = sum(a)
if (abs(b-6) > 1e-5) error stop

contains

    real function sum(a) result(r)
    real, intent(in) :: a(:)
    integer :: i
    print *, "sum"
    r = 0
    do i = 1, size(a)
        r = r + a(i)
    end do
    end function

    real function abs(a) result(r)
    real, intent(in) :: a
    print *, "abs"
    if (a > 0) then
        r = a
    else
        r = -a
    end if
    end function

end
