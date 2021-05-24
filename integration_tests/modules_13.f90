module module_13
implicit none

contains

integer function f1()
    f1 = f2()
end function

integer function f2()
    f2 = 5
end function

end module module_13

program main
use module_13
implicit none
integer :: f
f = f1()
print *, f
end program main
