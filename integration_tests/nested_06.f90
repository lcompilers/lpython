module nested_06_a
implicit none

contains

subroutine b(x)
real :: x 
print *, "b()"
call c()
contains
    subroutine c()
    print *, 5
    print *, x
    end subroutine c
end subroutine b

end module

program nested_06
use nested_06_a, only: b
implicit none

call b(6.0)

end
