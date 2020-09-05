subroutine f(a, b)
integer, intent(inout) :: a
integer, intent(in out) :: b
a = b + 1
b = a + 1
end subroutine
