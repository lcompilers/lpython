program functions_02
implicit none
integer :: i
real :: f, g
i = 1
f = 3.5

g = mysum(i, f)

print *, g

contains

    real function mysum(a, b) result(c)
    integer, intent(in) :: a
    real, intent(in) :: b
    c = a + b
    end function

end program
