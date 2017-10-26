module subroutine1
implicit none

contains

subroutine some_subroutine
integer :: a, b, c
a = 1
b = 8
end subroutine

subroutine some_other_subroutine
integer :: x, y
y = 5
x = y
end subroutine

end module
