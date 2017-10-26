module expr1
implicit none

contains

subroutine a
integer :: a, b
a = 1+2*3
b = (1+2+a)*3
end subroutine

end module
