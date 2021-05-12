module module_12
implicit none
integer :: A(5)
end module


program prog_module_12
use module_12
implicit none
A(5) = 5
if (A(5) /= 5) error stop
end program
