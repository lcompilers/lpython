module a_01
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

program modules_01
use a_01, only: b
implicit none

call b()

end
