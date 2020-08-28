program subroutines_03
implicit none
integer :: i, j, k, l
i = 1
j = 1
k = 1
l = 1
call f(i, j, k, l)
if (j /= 4) error stop

call f(i, j, d=l, c=k)
if (j /= 4) error stop

call f(a=i, b=j, d=l, c=k)
if (j /= 4) error stop

call f(b=j, a=i, c=k, d=l)
if (j /= 4) error stop

contains

    subroutine f(a, b, c, d)
    integer, intent(in) :: a, c, d
    integer, intent(out) :: b
    b = a + 1 + c + d
    end subroutine

end program
