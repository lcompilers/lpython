module a_02
implicit none

contains

subroutine b()
print *, "b()"
end subroutine

end module

module c_02
implicit none

contains

subroutine d()
print *, "d()"
end subroutine

end module

program modules_02
use a_02
use c_02, only: x=>d
implicit none

call b()
call x()

end
