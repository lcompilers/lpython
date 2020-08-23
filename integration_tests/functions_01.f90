program functions_01
implicit none
integer :: i, j
i = 1
j = 1
if (j /= 1) error stop
j = f(i)
if (i /= 1) error stop
if (j /= 2) error stop

j = 1
if (j /= 1) error stop
j = f(3)
if (j /= 4) error stop

j = 1
if (j /= 1) error stop
j = f(1+2)
if (j /= 4) error stop

j = 1
if (j /= 1) error stop
j = f(i+2)
if (j /= 4) error stop

contains

    integer function f(a) result(b)
    integer, intent(in) :: a
    b = a + 1
    end function

end program
