program functions_03
implicit none
real(8) :: i, j
i = 1
j = 1
if (j /= 1) error stop
j = f(i)
if (i /= 1) error stop
if (j /= 2) error stop

j = 1
if (j /= 1) error stop
j = f(3.0_8)
if (j /= 4) error stop

j = 1
if (j /= 1) error stop
j = f(1.0_8 + 2.0_8)
if (j /= 4) error stop

j = 1
if (j /= 1) error stop
j = f(i + 2)
if (j /= 4) error stop

contains

    real(8) function f(a) result(b)
    real(8), intent(in) :: a
    b = a + 1
    end function

end program
