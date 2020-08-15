module a
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

module c
implicit none

contains

subroutine d()
print *, "d()"
end subroutine

end module

program modules_02
use a
use c, only: x=>d
implicit none

call b()
call x()

end
