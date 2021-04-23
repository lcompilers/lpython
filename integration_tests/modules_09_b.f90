module modules_09_b
implicit none
private
public b, i

integer, parameter :: i = 5

contains

integer function b()
b = 5
end function

end module
