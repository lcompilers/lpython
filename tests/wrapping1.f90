module wrapping1
implicit none
integer, parameter :: c_int = 4

contains

subroutine sub1(a, b) bind(c)
integer(c_int), intent(in) :: a
integer(c_int), intent(out) :: b
b = a + 1
end subroutine

end module
