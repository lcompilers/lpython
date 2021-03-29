module a_06
implicit none

contains

integer function b() result(r)
print *, "b()"
r = 5
end function

end module

program modules_06
use a_06, only: b
implicit none

integer :: i

i = b()

print *, i

end
