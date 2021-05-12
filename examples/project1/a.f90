module a
use b, only: g
implicit none
private
public f, fib

contains

integer function f()
f = 3 + g()
end function

subroutine fib(n)
integer, intent(in) :: n
integer :: i, a, b, c
a = 1
b = 1
do i = 1, n-2
    c = a+b
    print *, "Fibonacci: ", i, " ", c
    a = b;
    b = c;
end do
end subroutine

end module
