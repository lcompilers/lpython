module nested_05_a
implicit none

contains

subroutine b()
integer :: x = 6
real :: y = 5.5
print *, x
print *, y
call c()
print *, x
print *, y
contains
    subroutine c()
    print *, x
    print *, y
    x = 4
    y = 3.5
    end subroutine c
end subroutine b

end module

program nested_05
use nested_05_a, only: b
implicit none

call b()

end
