module nested_03_a
implicit none

contains

subroutine b()
real :: x = 6
print *, "b()"
call c()
contains
    subroutine c()
    print *, 5
    print *, x
    end subroutine c
end subroutine b

end module

program nested_03
use nested_03_a, only: b
implicit none

call b()

end
