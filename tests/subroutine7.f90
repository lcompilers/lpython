subroutine f(a, b)
intent(inout) :: a
intent(in out) :: b
a = b + 1
b = a + 1
end subroutine
