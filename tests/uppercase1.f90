module uppercase1
real, dimension(3) :: x, y

contains

subroutine sub(a)
integer, intent(inout) :: a
a = a + SIZE(x) + SIZE(y)
end subroutine

end module
