module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

program modules_01
use a, only: b
implicit none

call b()

end
