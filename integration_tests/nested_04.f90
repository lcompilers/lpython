module nested_04_a
implicit none

contains

integer function b(x)
integer, intent(in) :: x
integer y
real :: yy = 6.6
y = x
print *, "b()"
b = c(6)
contains
    integer function c(z)
    integer, intent(in) :: z
    print *, z
    print *, y
    print *, yy
    c = z
    end function c
end function b

end module

program nested_04
use nested_04_a, only: b
implicit none
integer test
test = b(5)

end
